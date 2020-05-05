/* cal source file */

#include <math.h>

#include "crc.h"

#include "app.h"
#include "app_i2c.h"
#include "app_cal.h"
#include "app_factory.h"
#include "app_key.h"
#include "app_data.h"

#define CAL_MAGIC_NUM 0x41415ABB
#define CAL_DATA_ADDR 0xFE00 /* last sector, for cal */

#define CAL_TEMP_LOW    37
#define CAL_TEMP_HIGH   42

/* flash sector size */
#define FLASE_SECTOR_SIZE 512

static FactoryData_t gstFactory;

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
    uint8_t *pBuffer = (uint8_t *)&pFactory->CalData;

    uMagic = FlashReadWord(uAddr);
    if (uMagic != CAL_MAGIC_NUM)
    {
        return FALSE;
    }

    ///< 加载产测数据
    pFactory->uMagic = uMagic;
    uAddr += sizeof(pFactory->uMagic);
    pFactory->u16Len = FlashReadShort(uAddr);
    uAddr += sizeof(pFactory->u16Len);
    pFactory->u16Crc = FlashReadShort(uAddr);
    uAddr += sizeof(pFactory->u16Crc);
    for (i = 0; i < sizeof(CalData_t); i++)
    {
        *(pBuffer + i) = FlashReadByte(uAddr + i);
    }

    ///< 检验和检查
    Csum = CRC16_Get8(pBuffer, sizeof(CalData_t));
    if (Csum != pFactory->u16Crc)
    {
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

    ASSERT(sizeof(FactoryData_t) <= I2C_CAL_SIZE);

    if (!app_i2c_read_data(I2C_CAL_ADDR, (uint8_t *)pFactory, sizeof(FactoryData_t)))
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
    Csum = CRC16_Get8((uint8_t *)&pFactory->CalData, sizeof(CalData_t));
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
    ASSERT(sizeof(FactoryData_t) <= I2C_CAL_SIZE);
    return app_i2c_write_data(I2C_CAL_ADDR, (uint8_t *)pFactory, sizeof(FactoryData_t));
}

static boolean_t AppCalStore(FactoryData_t *pFactory)
{
    int i;
    uint32_t uAddr = CAL_DATA_ADDR;
    uint8_t *pBuffer = (uint8_t *)&pFactory->CalData;

    ASSERT(sizeof(FactoryData_t) <= FLASE_SECTOR_SIZE); //512Byte per Block

    ///< 关闭中断
    __disable_irq();

    ///< 擦除块
    while (Ok != Flash_SectorErase(uAddr))
        ;

    ///< 写入MAGIC
    Flash_WriteWord(uAddr, pFactory->uMagic);
    uAddr += sizeof(pFactory->uMagic);
    Flash_WriteHalfWord(uAddr, pFactory->u16Len);
    uAddr += sizeof(pFactory->u16Len);
    Flash_WriteHalfWord(uAddr, pFactory->u16Crc);
    uAddr += sizeof(pFactory->u16Crc);

    ///< 写入校准数据
    for (i = 0; i < sizeof(CalData_t); i++)
    {
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
    pFactory->u16Crc = CRC16_Get8((uint8_t *)pCal, sizeof(CalData_t));

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
    FactoryData_t I2cFactory = {0};
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
        if (pFactory->u16Crc != I2cFactory.u16Crc)
        {
            FlashCal_valid = FALSE;
        }
    }

    if (!I2cFac_valid) /* i2c invalid, 更新flash cal到i2c */
    {
        AppCalStoreToI2c(pFactory);
    }

    if (!FlashCal_valid) /* flash invalid, 更新i2c cal到flash */
    {
        memcpy(pFactory, &I2cFactory, sizeof(FactoryData_t));
        AppCalStore(pFactory);
    }

    ///< init Setup sensor type
    if(!NNA_SensorSet(pFactory->CalData.u8SensorType)) {
        return NULL;
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
    while (Ok != Flash_SectorErase(CAL_DATA_ADDR))
        ;

    ///< 覆盖MAGIC
    Flash_WriteWord(CAL_DATA_ADDR, 0xFFFFAAAA);

    ///< 还原中断
    __enable_irq();

    //擦除i2c factory
    app_i2c_write_data(I2C_CAL_ADDR, (uint8_t *)&gstFactory, sizeof(gstFactory));
}

///< 数据预处理(滤波)
#define SAMPLE_MAX (4)
#define SAMPLE_BUFF_SIZE    (SAMPLE_MAX + 1)
static void SampleInsert(uint32_t *aSum, uint32_t uVal)
{
    int i;

    if(aSum[0] < SAMPLE_MAX) {
        aSum[0]++;
    }
    for(i=aSum[0]; i>1; i--)
    {
        aSum[i] = aSum[i-1];
    }
    aSum[1] = uVal;
}

static uint32_t SampleMeans(uint32_t *aSum)
{
    int i;
    uint32_t uMeans = 0;

    for(i = 1; i <= aSum[0]; i++)
    {
        uMeans += aSum[i];
    }
    return (uMeans /= aSum[0]); // 均值
}

static uint32_t SampleVariance(uint32_t *aSum)
{
    int i;
    uint32_t uMeans, uEpison = 0;

    uMeans = SampleMeans(aSum);

    for(i = 1; i <= aSum[0]; i++) {
        if(uMeans > aSum[i]) {
            uEpison += pow((uMeans - aSum[i]), 2);
        } else {
            uEpison += pow((aSum[i] - uMeans), 2);
        }
    }

    return sqrt(uEpison); // 方差
}

static uint32_t SampleCal(uint32_t *aSum)
{
    uint8_t i;
    uint32_t uVal = 0, uMin = 0xFFFFFFFF, uMax = 0;

    for (i = 1; i <= aSum[0]; i++)
    {
        uVal += aSum[i];
        if (uMin > aSum[i])
        {
            uMin = aSum[i];
        }
        if (uMax < aSum[i])
        {
            uMax = aSum[i];
        }
    }

    uVal -= (uMax + uMin);

    return (uVal / (aSum[0] - 2)) * 2500 / 4096;
}

static void SampleDump(uint32_t *aSum)
{
    int8_t i;

    DBG_PRINT("N: %u\r\n   ", aSum[0]);
    for (i = 1; i <= aSum[0]; i++)
    {
        DBG_PRINT(" %u", aSum[i]);
#ifdef FACTORY_MODE_UV_DEBUG
        if(factory_mode) {
            AppLcdSetRawNumber(aSum[i], FALSE, 4);
            AppLcdDisplayUpdate(500);
        }
#endif
    }
    DBG_PRINT("\r\n");
}

///< ADC 修正值获取
boolean_t AppAdcCodeGet(uint32_t *uViR, uint32_t *uVNtcH, uint32_t *uVNtcL)
{
    int iSampleCount = SAMPLE_MAX;
    uint32_t uSumViR[SAMPLE_BUFF_SIZE], uSumVNtcH[SAMPLE_BUFF_SIZE], uSumVNtcL[SAMPLE_BUFF_SIZE];

    delay1ms(100); /* 等适应了再采集数据 */

    ///<*** ADC数据采集
    uSumViR[0] = uSumVNtcH[0] = uSumVNtcL[0] = 0;
    while (iSampleCount--)
    {
        uint32_t uAdcCode;

        Sysctrl_SetPCLKDiv(SysctrlPclkDiv8);

        AppAdcVirAvgCodeGet(&uAdcCode); ///< 表面温度 ADC采样
        SampleInsert(uSumViR, uAdcCode);

        AppAdcNtcHAvgCodeGet(&uAdcCode); ///< 环境温度RH ADC采样
        SampleInsert(uSumVNtcH, uAdcCode);

        AppAdcNtcLAvgCodeGet(&uAdcCode); ///< 表面温度RL ADC采样
        SampleInsert(uSumVNtcL, uAdcCode);

        Sysctrl_SetPCLKDiv(SysctrlPclkDiv1);

        delay1ms(20);
    }

    SampleDump(uSumViR);

    *uViR = SampleCal(uSumViR);
    *uVNtcH = SampleCal(uSumVNtcH);
    *uVNtcL = SampleCal(uSumVNtcL);

    DBG_PRINT("\tADC: %u H-L: %u %u\r\n", *uViR, *uVNtcH, *uVNtcL);

    return TRUE;
}

///< ADC 采样及温度计算
// TRUE  - 标准模式(使用标定后的值)
// FALSE - 标定(测试)模式
boolean_t AppTempCalculate(CalData_t *pCal,
                           uint32_t *uTNtc,
                           uint32_t *uTSurface,
                           uint32_t *uTHuman,
                           uint32_t *pViR)
{
    static int i = 0;
    uint32_t u32SampIndex;         ///< 采样次数
    uint32_t uViR, uRa, uVNtcH, uVNtcL; ///< ADC 采样值
    float32_t fNtcTemp, fSurfaceTemp, fSkinTemp, fHumanTemp;

    ASSERT(pCal);

    // it looks like this embedded processor
    // cannot be intterrupted in float processing
    __disable_irq();

    if (FALSE == AppAdcCodeGet(&uViR, &uVNtcH, &uVNtcL))
    {
        __enable_irq();
        return FALSE;
    }

    ///< 环境温度获取
    fNtcTemp = NNA_NtcTempGet(uVNtcH, uVNtcL, &uRa); ///< NTC 环境温度值获取
    if (uTNtc)
        *uTNtc = (uint32_t)(fNtcTemp * 100);

    ///< 物体表面
    fSurfaceTemp = NNA_SurfaceTempGet(pCal, fNtcTemp, uViR, 1.0);
    if (uTSurface)
        *uTSurface = (uint32_t)(fSurfaceTemp * 100);

    // 人体体表
    fSkinTemp = fSurfaceTemp;
    if (fSkinTemp < 36)
    {
        fSkinTemp = NNA_SurfaceTempGet(pCal, fNtcTemp, uViR, 0.98295);
    }

    ///< 人体温度
    fHumanTemp = NNA_HumanBodyTempGet(pCal, fNtcTemp, fSkinTemp);
    if (uTHuman)
        *uTHuman = (uint32_t)(fHumanTemp * 100);

    if (pViR)
        *pViR = uViR;

    DBG_PRINT("ViR: %u Ntc: %2.2f Surf: %2.2f Skin: %2.2f Hum: %2.2f\r\n",
              uViR, fNtcTemp, fSurfaceTemp, fSkinTemp, fHumanTemp);

    __enable_irq();
    return TRUE;
}

static boolean_t AppCaliTargetTemp(CalData_t *pCal, uint8_t uTargetTemp)
{
    boolean_t bSuCali;
    float32_t fNtc, fTemp;
    uint32_t uNtcH = 0, uNtcL = 0, uViR = 0, uRa = 0;
    uint32_t aSampleViR[SAMPLE_BUFF_SIZE] = {0,}, uReTry = 0, uAcc;

    do {
        ///< 读取ADC
        if (!AppAdcCodeGet(&uViR, &uNtcH, &uNtcL))
        {
            AppLcdSetLock(TRUE);
            AppLcdDisplayUpdate(200);
            continue;
        }
        SampleInsert(aSampleViR, uViR);

        AppLcdSetLock(FALSE);
        AppLcdSetRawNumber(uViR, FALSE, 4);
        AppLcdSetLogIndex(FALSE, uTargetTemp);
        AppLcdDisplayUpdate(0);

        ///< 环境温度
        // it looks like this embedded processor
        // cannot be intterrupted in float processing
        __disable_irq();
        bSuCali = NNA_Calibration(pCal, 
                                    NNA_NtcTempGet(uNtcH, uNtcL, &uRa),
                                    uTargetTemp,
                                    &fTemp,
                                    uViR);
        __enable_irq();

        if(!bSuCali) 
        {
            AppLcdSetLock(TRUE);
            AppLcdDisplayUpdate(200);
            return FALSE;
        }

        /* log set Ra, uViR */
        AppLcdSetLogRawNumber((uRa / 100), TRUE, 1);
        AppLcdDisplayUpdate(0);

        uAcc = SampleVariance(aSampleViR);
        if(uAcc == 0 && uReTry > 2)
        {
            return TRUE;
        }
        else
        {
            DBG_PRINT("\t aVariance: %u\r\n", uAcc);
        }
    } while (uReTry++ < 10);

    return FALSE;
}

///< 校准(标定)模式API
void AppCalibration(void)
{
    CalData_t Cal;
    uint8_t uCaliTarget = CAL_TEMP_LOW;

    NNA_CalInit(&Cal);

    AppLedEnable(LedOrange);
    AppLcdClearAll();
    AppLcdBlink();

    ///< 当前软件版本
    AppLcdSetRawNumber(SYS_SW_VERSION, TRUE, 4);
    ///< 选择传感器
    Cal.u8SensorType = NNA_SensorGetIndex();
    NNA_SensorSet(Cal.u8SensorType);
    AppLcdSetLogRawNumber(NNA_SensorGet(), FALSE, 4);
    AppLcdDisplayUpdate(0);

    while (!key_pressed_query(KEY_TRIGGER)); //等按键触发

    do {
        if(key_pressed_query(KEY_MINUS)) {
            Cal.u8SensorType ++;
            if(Cal.u8SensorType >= en_sensor_max) {
                Cal.u8SensorType = 0;
            }
            if(Cal.u8SensorType % 2) {
                AppLedEnable(LedOrange);
            } else {
                AppLedEnable(LedGreen);
            }
        }

        ///< 设置当前传感器选择
        NNA_SensorSet(Cal.u8SensorType);
        AppLcdSetRawNumber(NNA_SensorGet(), FALSE, 4);
        AppLcdSetLogRawNumber(NNA_SensorGet(), FALSE, 4);
        AppLcdDisplayUpdate(150);
    } while (key_pressed_query(KEY_TRIGGER)); //等按键释放

    ///< 校准
    while (1)
    {
        while (!key_pressed_query(KEY_TRIGGER)); //等按键触发

        if(AppCaliTargetTemp(&Cal, uCaliTarget))
        {
            beep_once(100);
            if(uCaliTarget == CAL_TEMP_HIGH) 
            {
                break;
            }
            else
            {
                uCaliTarget = CAL_TEMP_HIGH;
            }
        } else {
            beep_once(800);
        }

        while (key_pressed_query(KEY_TRIGGER)); //等按键释放
    }

    ///< 回测
    while (1)
    {
        uint32_t uNtc, uSurf, uHuman, uViR = 0;
        ///< 用校准后的参数验证测试 & 按键确认
        while (!key_pressed_query(KEY_TRIGGER) && !key_pressed_query(KEY_FN))
            ; //等按键触发

        if (key_pressed_query(KEY_TRIGGER))
        {
            AppTempCalculate(&Cal, &uNtc, &uSurf, &uHuman, &uViR);
            ///< 打出环境温度
            AppLcdSetLogIndex(FALSE, uNtc / 100);
            AppLcdSetTemp(uSurf / 10);
            /* log set uViR */
            AppLcdSetLogRawNumber(uViR, FALSE, 1);
            AppLcdDisplayUpdate(0);
        }

        if (key_pressed_query(KEY_FN))
        {
            break;
        }

        //while(key_pressed_query(KEY_TRIGGER) || key_pressed_query(KEY_FN)); //等按键释放
    }

    AppBeepBlink((SystemCoreClock / 1000));
    AppLcdClearAll();
    AppLcdDisplayUpdate(100);
    ///< 回写校准数据
    AppCalUpdateAndSaveFactory(&Cal);
}

void AppCalInit()
{
    // hold [FN] on PWRON to force calibration mode
    if (key_pressed_query(KEY_FN))
    {
        AppCalLoad(); // 可能会用到上一次校准的传感器类型
        AppCalibration();
    }
    else
    {
        // if no cal data stored, goto calibration
        if (AppCalLoad() == NULL)
        {
            AppCalibration();
        }
    }
    return;
}

#define CAL_DEBUG 0
#if CAL_DEBUG
void cal_debug()
{
    CalData_t Cal;
    CalData_t *pCal = NULL;
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
    Cal.uVAdcH = 12;
    Cal.uVAdcL = 34;
    Cal.u8HumanFix = 29;
    AppCalUpdateAndSaveFactory(&Cal);

    if (!CalLoad(&Fact))
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
    if (!CalLoadFromI2c(&Fact))
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
    while (Ok != Flash_SectorErase(CAL_DATA_ADDR))
        ;

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
    if (!CalLoad(&Fact))
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
    app_i2c_write_data(I2C_CAL_ADDR, (uint8_t *)&Fact, sizeof(FactoryData_t));

    pCal = AppCalLoad(); //flash cal write to i2c
    if (pCal == NULL)
    {
        printf("%s:%u error!\r\n", __func__, __LINE__);
        return;
    }

    memset(&Fact, 0, sizeof(FactoryData_t));
    if (!CalLoadFromI2c(&Fact))
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
