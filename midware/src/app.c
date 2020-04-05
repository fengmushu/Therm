/******************************************************************************
*Copyright(C)2018, Huada Semiconductor Co.,Ltd All rights reserved.
*
* This software is owned and published by:
* Huada Semiconductor Co.,Ltd("HDSC").
*
* BY DOWNLOADING, INSTALLING OR USING THIS SOFTWARE, YOU AGREE TO BE BOUND
* BY ALL THE TERMS AND CONDITIONS OF THIS AGREEMENT.
*
* This software contains source code for use with HDSC
* components. This software is licensed by HDSC to be adapted only
* for use in systems utilizing HDSC components. HDSC shall not be
* responsible for misuse or illegal use of this software for devices not
* supported herein. HDSC is providing this software "AS IS" and will
* not be responsible for issues arising from incorrect user implementation
* of the software.
*
* Disclaimer:
* HDSC MAKES NO WARRANTY, EXPRESS OR IMPLIED, ARISING BY LAW OR OTHERWISE,
* REGARDING THE SOFTWARE (INCLUDING ANY ACOOMPANYING WRITTEN MATERIALS),
* ITS PERFORMANCE OR SUITABILITY FOR YOUR INTENDED USE, INCLUDING,
* WITHOUT LIMITATION, THE IMPLIED WARRANTY OF MERCHANTABILITY, THE IMPLIED
* WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE OR USE, AND THE IMPLIED
* WARRANTY OF NONINFRINGEMENT.
* HDSC SHALL HAVE NO LIABILITY (WHETHER IN CONTRACT, WARRANTY, TORT,
* NEGLIGENCE OR OTHERWISE) FOR ANY DAMAGES WHATSOEVER (INCLUDING, WITHOUT
* LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION,
* LOSS OF BUSINESS INFORMATION, OR OTHER PECUNIARY LOSS) ARISING FROM USE OR
* INABILITY TO USE THE SOFTWARE, INCLUDING, WITHOUT LIMITATION, ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOSS OF DATA,
* SAVINGS OR PROFITS,
* EVEN IF Disclaimer HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* YOU ASSUME ALL RESPONSIBILITIES FOR SELECTION OF THE SOFTWARE TO ACHIEVE YOUR
* INTENDED RESULTS, AND FOR THE INSTALLATION OF, USE OF, AND RESULTS OBTAINED
* FROM, THE SOFTWARE.
*
* This software may be replicated in part or whole for the licensed use,
* with the restriction that this Disclaimer and Copyright notice must be
* included with each copy of this software, whether used in part or whole,
* at all times.
*/

/** \file app.c
 **
 ** Common API.
 ** @link flashGroup Some description @endlink
 **
 **   - 2018-05-08
 **
 ******************************************************************************/

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "app.h"

/**
 *******************************************************************************
 ** \addtogroup FlashGroup
 ******************************************************************************/
//@{

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define FREQBEEPVAL             (1200) //Hz
 
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

static UID_t           gstUID;
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

static void AppLoadUID(void)
{
    int i;

    memset(&gstUID, 0, sizeof(gstUID));
    for(i=0; i<sizeof(UID_t); i++) {
        *((uint8_t*)&gstUID + i) = *(uint8_t*)(UID_BASE_ADDR);
    }

    DBG_PRINT("RevID: %2x - %2x, %2x, %2x\r\n", gstUID.RevID, \
            gstUID.WaterNumber, gstUID.XCoordWater, gstUID.YCoordWater);
    DBG_PRINT("\t%02x-%02x-%02x-%02x-%02x-%02x\r\n", \
            gstUID.LotNumber[0], gstUID.LotNumber[1], gstUID.LotNumber[2],
            gstUID.LotNumber[3], gstUID.LotNumber[4], gstUID.LotNumber[5]);
}

static boolean_t CalLoad(void)
{
    int i;
    uint32_t uMagic;
    uint16_t Csum;
    uint32_t uAddr = CAL_DATA_ADDR;
    FactoryData_t *pFactory = &gstFactory;
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

    return TRUE;
}

CalData_t *AppCalGet(void)
{
    static uint16_t u16CRC = 0xFFFF;
    FactoryData_t *pFactory = &gstFactory;

    ///< from cacahed data
    if(u16CRC == pFactory->u16Crc) {
        return &pFactory->CalData;
    }

    ///< load from flash
    if(CalLoad()) {
        u16CRC = pFactory->u16Crc;
        return &pFactory->CalData;
    } else {
        return NULL;
    }
}

boolean_t AppCalStore(CalData_t *pCal)
{
    int i;
    uint16_t u16Crc, u16Len;
    uint32_t uAddr = CAL_DATA_ADDR;
    FactoryData_t *pFactory = &gstFactory;
    uint8_t *pBuffer = (uint8_t*)pCal;

    ASSERT(sizeof(CalData_t) <= 58); //64Byte per Block

    ///< 关闭中断
    __disable_irq();
	
    ///< 写入工厂信息头部
    u16Crc = CRC16_Get8((uint8_t*)pCal, sizeof(CalData_t));
    if(u16Crc == 0) {
        ///< 不处理这种特殊情况(数据区可能都全是0)
        __enable_irq();
        return FALSE;
    }

    ///< 擦除块
    while(Ok != Flash_SectorErase(uAddr));

    pFactory->uMagic = CAL_MAGIC_NUM;
    pFactory->u16Len = sizeof(CalData_t);
    pFactory->u16Crc = u16Crc;

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
}

void AppParaAreaInit(void)
{
    // M0P_SYSCTRL->PERI_CLKEN_f.FLASH = 1;
    Sysctrl_SetPeripheralGate(SysctrlPeripheralFlash, TRUE);

    ///< 初始化Flash
    Flash_Init(1, TRUE);

    ///< 加载UID
    AppLoadUID();
}

void AppBeepBlink(uint32_t u32FreqIndex)
{    
    while(u32FreqIndex--)
    {
        if((u32FreqIndex/((SystemCoreClock/FREQBEEPVAL)>>1))&0x01)
        {
            Gpio_ClrIO(M_BEEP_PORT, M_BEEP_PIN);
        }
        else
        {
            Gpio_SetIO(M_BEEP_PORT, M_BEEP_PIN);
        }
    
    }
    Gpio_ClrIO(M_BEEP_PORT, M_BEEP_PIN);
    
}

///< 定时关机模块初始化
void AppPowerOffModuleInit(void)
{
    stc_rtc_initstruct_t RtcInitStruct;
    stc_rtc_alarmtime_t RtcAlmStruct;

    DDL_ZERO_STRUCT(RtcInitStruct);                      //变量初始值置零
    DDL_ZERO_STRUCT(RtcAlmStruct);

    // Sysctrl_SetPeripheralGate(SysctrlPeripheralRtc,TRUE);//RTC模块时钟打开
    // Sysctrl_ClkSourceEnable(SysctrlClkRCL, TRUE);        //使能内部RCL时钟作为RTC时钟

    RtcInitStruct.rtcAmpm = RtcPm;                       //12小时制
    RtcInitStruct.rtcClksrc = RtcClkRcl;                 //内部低速时钟
    RtcInitStruct.rtcPrdsel.rtcPrdsel = RtcPrds;         //周期中断类型PRDS
    RtcInitStruct.rtcPrdsel.rtcPrds   = RtcNone;           //不产生周期中断
    RtcInitStruct.rtcTime.u8Second    = 0x00;               //配置RTC时间2019年4月17日10:01:55
    RtcInitStruct.rtcTime.u8Minute    = 0x00;
    RtcInitStruct.rtcTime.u8Hour      = 0x20;
    RtcInitStruct.rtcTime.u8Day       = 0x20;
    RtcInitStruct.rtcTime.u8DayOfWeek = 0x04;
    RtcInitStruct.rtcTime.u8Month     = 0x20;
    RtcInitStruct.rtcTime.u8Year      = 0x20;
    RtcInitStruct.rtcCompen           = RtcCompenDisable;           // 使能时钟误差补偿
    RtcInitStruct.rtcCompValue        = 0;                          //补偿值  根据实际情况进行补偿
    Rtc_Init(&RtcInitStruct);

    RtcAlmStruct.RtcAlarmMinute = 0x01;
    RtcAlmStruct.RtcAlarmHour   = 0x20;
    RtcAlmStruct.RtcAlarmWeek   = 0x7f;                  //从周一到周日，每天10:02:05启动一次闹铃
    Rtc_SetAlarmTime(&RtcAlmStruct);                     //配置闹铃时间
    Rtc_AlmIeCmd(TRUE);                                  //使能闹钟中断

    EnableNvic(RTC_IRQn, IrqLevel3, TRUE);               //使能RTC中断向量
    Rtc_Cmd(TRUE);                                       //使能RTC开始计数
}

en_result_t AppRtcFeed(void)
{
    en_result_t enRet = Ok;
    uint16_t u16TimeOut;
    u16TimeOut = 1000;
    if(M0P_RTC->CR0_f.START == 1)
    {
        M0P_RTC->CR1_f.WAIT = 1;
        while(--u16TimeOut)
        {
            if(M0P_RTC->CR1_f.WAITF)
            {
                    break;
            }
        }
        if(u16TimeOut==0)
        {
            return ErrorTimeout;
        }
    }
    M0P_RTC->SEC   = 0;
    M0P_RTC->MIN   = 0;

    M0P_RTC->CR1_f.WAIT = 0;
    if(M0P_RTC->CR0_f.START == 1)
    {
        while(M0P_RTC->CR1_f.WAITF)
        {}
    }
    enRet = Ok;
    return enRet;
}


void AppLedEnable(en_led_colour_t enLedColour)
{
    if(LedRed == enLedColour)
    {
        Gpio_SetIO(M_LED1_PORT, M_LED1_PIN);
        Gpio_SetIO(M_LED2_PORT, M_LED2_PIN);
        // Gpio_ClrIO(M_LED3_PORT, M_LED3_PIN);
    }
    else if(LedLightBlue == enLedColour)
    {
        Gpio_ClrIO(M_LED1_PORT, M_LED1_PIN);
        Gpio_ClrIO(M_LED2_PORT, M_LED2_PIN);
        // Gpio_SetIO(M_LED3_PORT, M_LED3_PIN);
    }
    else
    {
        if(Gpio_ReadOutputIO(M_LED1_PORT, M_LED1_PIN)) {
            Gpio_ClrIO(M_LED1_PORT, M_LED1_PIN);
            Gpio_ClrIO(M_LED2_PORT, M_LED2_PIN);
        } else {
            Gpio_SetIO(M_LED1_PORT, M_LED1_PIN);
            Gpio_SetIO(M_LED2_PORT, M_LED2_PIN);
        }
    }
}

void AppLedDisable(void)
{
    Gpio_SetIO(M_LED1_PORT, M_LED1_PIN);
    Gpio_SetIO(M_LED2_PORT, M_LED2_PIN);
    // Gpio_SetIO(M_LED3_PORT, M_LED3_PIN);
}

///< VCC 电量监测模块初始化
void AppVolMonitorInit(void)
{
    stc_lvd_cfg_t stcLvdCfg;

    DDL_ZERO_STRUCT(stcLvdCfg);     //变量清0

    Sysctrl_SetPeripheralGate(SysctrlPeripheralVcLvd, TRUE);    //开LVD时钟

    stcLvdCfg.enAct        = LvdActMskInt;              ///< 配置触发产生中断
    stcLvdCfg.enInputSrc   = LvdInputSrcMskVCC;         ///< 配置LVD输入源
    stcLvdCfg.enThreshold  = LvdMskTH2_7V;              ///< 配置LVD基准电压
    stcLvdCfg.enFilter     = LvdFilterMskEnable;        ///< 滤波使能
    stcLvdCfg.enFilterTime = LvdFilterMsk28_8ms;        ///< 滤波时间设置
    stcLvdCfg.enIrqType    = LvdIrqMskHigh;             ///< 中断触发类型
    Lvd_Init(&stcLvdCfg);
    
    ///< 中断开启
    Lvd_EnableIrq();
    Lvd_ClearIrq();
    EnableNvic(LVD_IRQn, IrqLevel3, TRUE);              ///< NVIC 中断使能
    
    ///< LVD 模块使能
    Lvd_Enable();
}

///< 串口模块配置
void AppUartInit(void)
{
    stc_uart_cfg_t  stcCfg;
    stc_uart_multimode_t stcMulti;
    stc_uart_baud_t stcBaud;

    DDL_ZERO_STRUCT(stcCfg);
    DDL_ZERO_STRUCT(stcMulti);
    DDL_ZERO_STRUCT(stcBaud);
    
    Sysctrl_SetPeripheralGate(SysctrlPeripheralUart1,TRUE);           //UART0/1 外设模块时钟使能
    
    stcCfg.enRunMode        = UartMskMode1;                     //模式1
    stcCfg.enStopBit        = UartMsk1bit;                      //1位停止位
    stcCfg.stcBaud.u32Baud  = 115200;                           //波特率115200
    stcCfg.stcBaud.enClkDiv = UartMsk8Or16Div;                  //通道采样分频配置
    stcCfg.stcBaud.u32Pclk  = Sysctrl_GetPClkFreq();            //获得外设时钟（PCLK）频率值
    Uart_Init(DBG_CONSOLE, &stcCfg);                              //串口初始化

    Uart_ClrStatus(DBG_CONSOLE, UartRC);                          //清接收请求
    Uart_ClrStatus(DBG_CONSOLE, UartTC);                          //清发送请求
    Uart_EnableIrq(DBG_CONSOLE, UartRxIrq);                       //使能串口接收中断
    Uart_EnableIrq(DBG_CONSOLE, UartTxIrq);                       //使能串口发送中断
}

void hexdump(void *p, int len)
{

}

//@} // BgrGroup

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
