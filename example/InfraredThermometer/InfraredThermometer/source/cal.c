/* cal source file */

#include "app.h"
#include "app_i2c.h"
#include "cal.h"
#include "crc.h"

#define CAL_MAGIC_NUM           0x41415ABB
#define CAL_DATA_ADDR           0xFE00 /* last sector, for cal */

/* cal data 在eeprom 里的存放位置 */
#define I2C_CAL_DATA_ADDR    0xC0
#define I2C_CAL_DATA_SIZE    0x40

/* flash sector size */
#define FLASE_SECTOR_SIZE       512


static FactoryData_t   gstFactory;


static inline uint32_t FlashReadWord(uint32_t uAddr)
{
    return *((uint32_t *)uAddr);
}

static inline uint16_t FlashReadShort(uint32_t uAddr)
{
    return *((uint16_t *)uAddr);
}

static inline uint8_t FlashReadByte(uint32_t uAddr)
{
    return *((uint8_t *)uAddr);
}

static boolean_t CalLoad(FactoryData_t *pFactory)
{
    int i;
    uint32_t uMagic;
    uint16_t Csum;
    uint32_t uAddr = CAL_DATA_ADDR;
    uint8_t  *pBuffer = (uint8_t*)&pFactory->CalData;

    uMagic = FlashReadWord(uAddr);
    if(uMagic != CAL_MAGIC_NUM) {
        return FALSE;
    }

    ///< 加载产测数据
    pFactory->uMagic = uMagic;
    uAddr += sizeof(pFactory->uMagic);
    pFactory->u16Len = FlashReadShort(uAddr);
    uAddr += sizeof(pFactory->u16Len);
    pFactory->u16Crc = FlashReadShort(uAddr);
    uAddr += sizeof(pFactory->u16Crc);
    for(i=0; i<sizeof(CalData_t); i++) {
        *(pBuffer + i) = FlashReadByte(uAddr + i);
    }

    ///< 检验和检查
    Csum = CRC16_Get8(pBuffer, sizeof(CalData_t));
    if(Csum != pFactory->u16Crc) {
        return FALSE;
    }

    ///< DUMP Cal
  #ifdef DEBUG
    {
        DBG_PRINT("### Cal Data:\r\n");
        DBG_PRINT("\tAMP: %f\r\n", pFactory->CalData.fAmp);
        DBG_PRINT("\tBase: %f\r\n", pFactory->CalData.fCalBase);
        DBG_PRINT("\tHuman: %u\r\n", pFactory->CalData.u8HumanFix);
    }
  #endif

    DBG_PRINT("%s Flash load OK\r\n", __func__);
  
    return TRUE;
}


static boolean_t CalLoadFromI2c(FactoryData_t *pFactory)
{
    uint16_t Csum;
    
    ASSERT(sizeof(FactoryData_t) <= I2C_CAL_DATA_SIZE);
    
    if (!app_i2c_read_data(I2C_CAL_DATA_ADDR, (uint8_t*)pFactory, sizeof(FactoryData_t)))
    {
        DBG_PRINT("%s:%d read cal error\r\n", __func__, __LINE__);
        return FALSE;
    }
    
    /* check magic */
    if (pFactory->uMagic != CAL_MAGIC_NUM)
    {
        DBG_PRINT("%s:%d magic number error\r\n", __func__, __LINE__);
        return FALSE;
    }
    
    /* check crc */
    Csum = CRC16_Get8((uint8_t*)&pFactory->CalData, sizeof(CalData_t));
    if (pFactory->u16Crc != Csum)
    {
        DBG_PRINT("%s:%d crc error\r\n", __func__, __LINE__);
        return FALSE;
    }

    ///< DUMP Cal
 #ifdef DEBUG
       {
           DBG_PRINT("### Cal Data in I2c:\r\n");
           DBG_PRINT("\tAMP: %f\r\n", pFactory->CalData.fAmp);
           DBG_PRINT("\tBase: %f\r\n", pFactory->CalData.fCalBase);
           DBG_PRINT("\tHuman: %u\r\n", pFactory->CalData.u8HumanFix);
       }
 #endif


    DBG_PRINT("%s load OK\r\n", __func__);
    return TRUE;
}

static boolean_t AppCalStoreToI2c(FactoryData_t *pFactory)
{
    ASSERT(sizeof(FactoryData_t) <= I2C_CAL_DATA_SIZE);
    return app_i2c_write_data(I2C_CAL_DATA_ADDR, (uint8_t*)pFactory, sizeof(FactoryData_t));    
}

static boolean_t AppCalStore(FactoryData_t *pFactory)
{
    int i;
    uint32_t uAddr = CAL_DATA_ADDR;
    uint8_t *pBuffer = (uint8_t*)&pFactory->CalData;

    ASSERT(sizeof(FactoryData_t) <= FLASE_SECTOR_SIZE); //512Byte per Block

    ///< 关闭中断
    __disable_irq();

    ///< 擦除块
    while(Ok != Flash_SectorErase(uAddr));

    ///< 写入MAGIC
    Flash_WriteWord(uAddr, pFactory->uMagic);
    uAddr += sizeof(pFactory->uMagic);
    Flash_WriteHalfWord(uAddr, pFactory->u16Len);
    uAddr += sizeof(pFactory->u16Len);
    Flash_WriteHalfWord(uAddr, pFactory->u16Crc);
    uAddr += sizeof(pFactory->u16Crc);

    ///< 写入校准数据
    for(i=0; i<sizeof(CalData_t); i++) {
        Flash_WriteByte(uAddr + i, *(pBuffer + i));
    }


    ///< 还原中断
    __enable_irq();

    return TRUE;
}

void AppCalUpdateAndSaveFactory(CalData_t *pCal)
{
    FactoryData_t *pFactory = &gstFactory;
    
    pFactory->uMagic = CAL_MAGIC_NUM;
    pFactory->u16Len = sizeof(CalData_t);

    ///< 写入工厂信息头部
    pFactory->u16Crc = CRC16_Get8((uint8_t*)pCal, sizeof(CalData_t));

    memcpy(&pFactory->CalData, pCal, sizeof(CalData_t));

    AppCalStore(pFactory);
    AppCalStoreToI2c(pFactory);
    
    return;
}

CalData_t *AppCalGet(void)
{
    return &gstFactory.CalData;
}

CalData_t *AppCalLoad(void)
{    
    FactoryData_t *pFactory = &gstFactory;
    FactoryData_t I2cFactory = { 0 };
    boolean_t FlashCal_valid = FALSE;
    boolean_t I2cFac_valid = FALSE;

    I2cFac_valid = CalLoadFromI2c(&I2cFactory);

    ///< load from flash
    FlashCal_valid = CalLoad(pFactory);

    /* check flash and i2c cal data */
    if (!FlashCal_valid && !I2cFac_valid)
    {
        return NULL;
    }
    
    if (FlashCal_valid && I2cFac_valid)
    {
        /* crc不一致，以i2c为准 */
        if(pFactory->u16Crc != I2cFactory.u16Crc)
        {
            FlashCal_valid = FALSE;
        }
    }
  
    if (!I2cFac_valid) /* i2c invalid, 更新flash cal到i2c */
    {
        AppCalStoreToI2c(pFactory);
    }

    if(!FlashCal_valid) /* flash invalid, 更新i2c cal到flash */
    {
        memcpy(pFactory, &I2cFactory, sizeof(FactoryData_t));
        AppCalStore(pFactory);
    }    

    return &pFactory->CalData;
}

void AppCalClean(void)
{
    ///< 清空缓存数据
    memset(&gstFactory, 0, sizeof(FactoryData_t));

    ///< 关闭中断
    __disable_irq();

    ///< 擦除块
    while(Ok != Flash_SectorErase(CAL_DATA_ADDR));

    ///< 覆盖MAGIC
    Flash_WriteWord(CAL_DATA_ADDR, 0xFFFFAAAA);

    ///< 还原中断
    __enable_irq();

    //擦除i2c factory
    app_i2c_write_data(I2C_CAL_DATA_ADDR, (uint8_t*)&gstFactory, sizeof(gstFactory));
    
}

#define CAL_DEBUG 0
#if CAL_DEBUG
void cal_debug()
{
    CalData_t  Cal;
    CalData_t* pCal = NULL;
    FactoryData_t Fact;
    
    AppCalClean();
    pCal = AppCalLoad();
    if (pCal != NULL)
    {
        printf("%s:%u error!\r\n", __func__, __LINE__);
        return;
    }

    Cal.fAmp = 0.1;
    Cal.fCalBase = 0.2;
    Cal.fTH = 0.3;
    Cal.fTL = 0.4;
    Cal.uVAdcH = 0x12;
    Cal.uVAdcL = 0x34;
    Cal.u8HumanFix = 0x29;
    AppCalUpdateAndSaveFactory(&Cal);
    
    if(!CalLoad(&Fact))
    {
        printf("%s:%u error!\r\n", __func__, __LINE__);
        return;
    }
    pCal = &gstFactory.CalData;
    if (memcmp(&Cal, pCal, sizeof(CalData_t)) != 0)
    {
        printf("%s:%u error!\r\n", __func__, __LINE__);
        return;
    }

    memset(&Fact, 0, sizeof(FactoryData_t));
    if(!CalLoadFromI2c(&Fact))
    {
        printf("%s:%u error!\r\n", __func__, __LINE__);
        return;
    }
    pCal = &Fact.CalData;
    if (memcmp(&Cal, pCal, sizeof(CalData_t)) != 0)
    {
        printf("%s:%u error!\r\n", __func__, __LINE__);
        return;
    }

    /* erase flash cal */
    //< 关闭中断
    __disable_irq();

    ///< 擦除块
    while(Ok != Flash_SectorErase(CAL_DATA_ADDR));

    ///< 覆盖MAGIC
    Flash_WriteWord(CAL_DATA_ADDR, 0xFFFFAAAA);
    __enable_irq();
    
    pCal = AppCalLoad(); //i2c cal write to flash
    if (pCal == NULL)
    {
        printf("%s:%u error!\r\n", __func__, __LINE__);
        return;
    }

    memset(&Fact, 0, sizeof(FactoryData_t));
    if(!CalLoad(&Fact))
    {
        printf("%s:%u error!\r\n", __func__, __LINE__);
        return;
    }
    pCal = &Fact.CalData;
    if (memcmp(&Cal, pCal, sizeof(CalData_t)) != 0)
    {
        printf("%s:%u error!\r\n", __func__, __LINE__);
        return;
    }
    
    /* erase i2c cal */
    memset(&Fact, 0, sizeof(FactoryData_t));
    app_i2c_write_data(I2C_CAL_DATA_ADDR, (uint8_t*)&Fact, sizeof(FactoryData_t));
    
    pCal = AppCalLoad(); //flash cal write to i2c
    if (pCal == NULL)
    {
        printf("%s:%u error!\r\n", __func__, __LINE__);
        return;
    }

    memset(&Fact, 0, sizeof(FactoryData_t));
    if(!CalLoadFromI2c(&Fact))
    {
        printf("%s:%u error!\r\n", __func__, __LINE__);
        return;
    }
    pCal = &Fact.CalData;
    if (memcmp(&Cal, pCal, sizeof(CalData_t)) != 0)
    {
        printf("%s:%u error!\r\n", __func__, __LINE__);
        return;
    }

    AppCalClean();        
           
}
#endif
