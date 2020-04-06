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
#include "app_lcd.h"
#include "app_i2c.h"
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
void AppRtcInit(void)
{
    stc_rtc_initstruct_t RtcInitStruct;
    stc_rtc_alarmtime_t RtcAlmStruct;

    DDL_ZERO_STRUCT(RtcInitStruct);                      //变量初始值置零
    DDL_ZERO_STRUCT(RtcAlmStruct);

    Sysctrl_ClkSourceEnable(SysctrlClkRCL, TRUE);        //使能内部RCL时钟作为RTC时钟
    Sysctrl_SetPeripheralGate(SysctrlPeripheralRtc,TRUE);//RTC模块时钟打开

    RtcInitStruct.rtcAmpm = RtcPm;                       //12小时制
    RtcInitStruct.rtcClksrc = RtcClkRcl;                 //内部低速时钟
    RtcInitStruct.rtcPrdsel.rtcPrdsel = RtcPrds;         //周期中断类型PRDS
    RtcInitStruct.rtcPrdsel.rtcPrds   = Rtc1S;           //不产生周期中断
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
    Rtc_AlmIeCmd(FALSE);                                  //使能闹钟中断

    EnableNvic(RTC_IRQn, IrqLevel3, TRUE);               //使能RTC中断向量
    Rtc_Cmd(TRUE);                                       //使能RTC开始计数
}

void AppRtcFeed(void)
{
    Rtc_Cmd(TRUE);
    AppSystemResume();

    ///< 清除超时计关机数器
    gnPowerOffTimeout = gnJiffies;
}

uint32_t AppRtcUpdate(void)
{
    ///< 判断自动关机是否超时
    if(time_after(gnJiffies, gnPowerOffTimeout + AUTO_PWROFF_TIMEOUT)) {
        ///< 关机
        gInSuspend = TRUE;
        AppSystemHalt();
    }

    return gnJiffies ++;
}

void AppSystemResume(void)
{
    if(gInSuspend) {
        AppLedEnable(1);
        AppBeepBlink((SystemCoreClock/500));

        AppLcdDisplayAll();
        AppLcdDisplayUpdate();

        Adc_Enable();
        Bgr_BgrEnable();
        gInSuspend = FALSE;
    }
}

void AppPmuInit(void)
{
    stc_lpm_config_t stcConfig;

    ///< 深度休眠模式下响应端口中断
    Gpio_SfIrqModeCfg(GpioSfIrqDpslpMode);

    ///< 低功耗模式配置
    stcConfig.enSEVONPEND   = SevPndEnable;
    stcConfig.enSLEEPDEEP   = SlpDpEnable;
    stcConfig.enSLEEPONEXIT = SlpExtDisable;
    Lpm_Config(&stcConfig);
}

void AppSystemHalt(void)
{
    ///< alert
    AppLedDisable();
    AppBeepBlink((SystemCoreClock/500));

    AppLcdClearAll();
    AppLcdDisplayUpdate();

    Adc_Disable();
    Bgr_BgrDisable();
    Lpm_GotoLpmMode();
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
    stcLvdCfg.enThreshold  = LvdMskTH2_5V;              ///< 配置LVD基准电压
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

#ifdef DEBUG
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
    return;
}
#endif //DEBUG

//@} // BgrGroup

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
