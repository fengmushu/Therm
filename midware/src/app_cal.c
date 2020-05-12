/* cal source file */

#include <math.h>

#include "crc.h"

#include "app.h"
#include "app_timer.h"
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

    return;
}

CalData_t *AppCalGet(void)
{
    return &gstFactory.CalData;
}

CalData_t *AppCalLoad(void)
{
    FactoryData_t *pFactory = &gstFactory;
    boolean_t FlashCal_valid = FALSE;

    ///< load from flash
    FlashCal_valid = CalLoad(pFactory);

    /* check flash and i2c cal data */
    if (!FlashCal_valid)
    {
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
}

static void SampleInsert(uint32_t *aSum, uint32_t uVal)
{
    aSum[++aSum[0]] = uVal;
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
    }
    DBG_PRINT("\r\n");
}

///< ADC 修正值获取
#define SAMPLE_MAX (4)
boolean_t AppAdcCodeGet(uint32_t *uViR, uint32_t *uVNtcH, uint32_t *uVNtcL)
{
    int iSampleCount = SAMPLE_MAX;
    uint32_t uSumViR[SAMPLE_MAX + 1], uSumVNtcH[SAMPLE_MAX + 1], uSumVNtcL[SAMPLE_MAX + 1];

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
    __enable_irq();

    return TRUE;
}

void AppCalibration(void)
{
    int i = 0;
    CalData_t Cal;

    AppLedEnable(LedOrange);
    AppLcdDisplayAll();

    do {

        // while(!key_pressed_query(KEY_TRIGGER)); //按下

        i = 1 - i;
        if(i)
        {
            AppLedEnable(LedOrange);
        }
        else
        {
            AppLedEnable(0);
        }

        AppLcdDisplayUpdate(40);

        // beep_once(100);
        DBG_PRINT("\t%u - %u\r\n", jffies_to_sec(), jffies_to_msc());

        // while(key_pressed_query(KEY_TRIGGER)); //释放

    } while (TRUE);

    ///< 回写校准数据
    AppCalUpdateAndSaveFactory(&Cal);
}

void AppCalInit()
{
    // hold [FN] on PWRON to force calibration mode
    if (key_pressed_query(KEY_TRIGGER))
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
