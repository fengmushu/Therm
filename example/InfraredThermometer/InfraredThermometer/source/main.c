/******************************************************************************
* Copyright (C) 2017, Huada Semiconductor Co.,Ltd All rights reserved.
*
* This software is owned and published by:
* Huada Semiconductor Co.,Ltd ("HDSC").
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
/******************************************************************************/
/** \file main.c
 **
 ** A detailed description is available at
 ** @link Sample Group Some description @endlink
 **
 **   - 2017-05-28 LiuHL    First Version
 **
 ******************************************************************************/

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "adc.h"
#include "gpio.h"
#include "bgr.h"
#include "lcd.h"
#include "lvd.h"
#include "rtc.h"
#include "lpm.h"
#include "uart.h"
#include "app_lcd.h"
#include "app_gpio.h"
#include "app_adc.h"
#include "app.h"
#include "nna.h"

/******************************************************************************
 * Local pre-processor symbols/macros ('#define')                            
 ******************************************************************************/

///< 
/******************************************************************************
 * Local type definitions ('typedef')                                         
 ******************************************************************************/
typedef enum enMState
{
    InitialMode     = 0u,
    TempMeasureMode = 1u,
    TempShowMode    = 2u,
    PowerOffMode    = 3u,
    MemoryMode      = 4u,
    TempScanMode    = 5u,
    PowerOnMode     = 6u,
    CalibrationMode = 7u
}enMState_t;

typedef enum enMKeyType
{
    Key_Left = 0u,
    Key_Mid,
    Key_Right,
    Key_Trig,
    Key_Switch,
    Key_Max,
}enMKeyType_t;

/******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
static uint32_t gu32AdcNtcResult = 0, gu32AdcVirResult = 0;
static uint32_t gu32UserKeyFlag = 0;
static enMState_t enMState = PowerOnMode;

#define KEY_LEFT()     (gu32UserKeyFlag & (1UL << Key_Left))
#define KEY_MID()      (gu32UserKeyFlag & (1UL << Key_Mid))
#define KEY_RIGHT()    (gu32UserKeyFlag & (1UL << Key_Right))
#define KEY_TRIG()     (gu32UserKeyFlag & (1UL << Key_Trig))
#define KEY_SWITCH()   (gu32UserKeyFlag & (1UL << Key_Switch))

#define KEY_SET_LEFT()     (gu32UserKeyFlag |= (1UL << Key_Left))
#define KEY_SET_MID()      (gu32UserKeyFlag |= (1UL << Key_Mid))
#define KEY_SET_RIGHT()    (gu32UserKeyFlag |= (1UL << Key_Right))
#define KEY_SET_TRIG()     (gu32UserKeyFlag |= (1UL << Key_Trig))
#define KEY_SET_SWITCH()   (gu32UserKeyFlag |= (1UL << Key_Switch))

#define KEY_CLR_LEFT()     (gu32UserKeyFlag &= ~(1UL << Key_Left))
#define KEY_CLR_MID()      (gu32UserKeyFlag &= ~(1UL << Key_Mid))
#define KEY_CLR_RIGHT()    (gu32UserKeyFlag &= ~(1UL << Key_Right))
#define KEY_CLR_TRIG()     (gu32UserKeyFlag &= ~(1UL << Key_Trig))
#define KEY_CLR_SWITCH()   (gu32UserKeyFlag &= ~(1UL << Key_Switch))

/******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
///< 环境温度、黑体温度、人体温度
static float32_t gf32NtcTemp, gfBlackTemp, gfSurfaceTemp, gfHumanTemp;
/******************************************************************************
 * Local variable definitions ('static')                                      *
 ******************************************************************************/
/*****************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
///< 串口发送重定向
int fputc(int ch, FILE * file)
{
    Uart_SendDataPoll(DBG_CONSOLE,ch);
    
    return ch;
}

///< App 系统时钟/总线初始化
void AppSysInit(void)
{
    stc_sysctrl_clk_cfg_t stcCfg;

    stcCfg.enClkSrc = SysctrlClkRCH;
    stcCfg.enHClkDiv = SysctrlHclkDiv1;
    stcCfg.enPClkDiv = SysctrlPclkDiv1;

    Sysctrl_ClkInit(&stcCfg);
    Sysctrl_ClkSourceEnable(SysctrlClkRCL, TRUE);           //使能内部RCL时钟作为RTC时钟
    Sysctrl_SetPeripheralGate(SysctrlPeripheralRtc,TRUE);   //RTC模块时钟打开
}

///< 系统初始化
void App_SystemInit(void)
{
    ///< 系统时钟/总线初始化
    AppSysInit();

    ///< GPIO 初始化
    AppMGpioInit();
    
    ///< ADC 模块初始化
    AppMAdcInit();
    
    ///< LCD 模块初始化
    AppLcdInit();
    
    ///< 电量监测模块初始化
    AppVolMonitorInit();
    
    ///< 自动关机模块
    AppPowerOffModuleInit();
    
    ///< 串口初始化
    AppUartInit();
    
    ///< 参数调整区初始化0
    AppParaAreaInit();
    
    // DBG_PRINT("Init Sucessful \r\n");
}


///< ADC 修正值获取
static boolean_t AppAdcCodeGet(uint32_t *uViR, uint32_t *uVNtcH, uint32_t *uVNtcL)
{
    uint32_t  u32SampIndex;     ///< 采样次数
    uint32_t  u32VirAdcCode, u32NtcHAdcCode, u32NtcLAdcCode;     ///< ADC 采样值
    uint32_t  u32VirAdcCodeAcc, u32NtcHAdcCodeAcc, u32NtcLAdcCodeAcc;       ///< ADC 累加值

    Gpio_SetIO(M_ADC_VBIRS_PORT, M_ADC_VBIRS_PIN); delay1ms(100);

    ///<*** ADC数据采集     
    {
        __disable_irq();

        u32SampIndex      = 0x10u;
        u32VirAdcCodeAcc  = 0;
        u32NtcHAdcCodeAcc = 0;
        u32NtcLAdcCodeAcc = 0;

        while(u32SampIndex--)
        {
            Sysctrl_SetPCLKDiv(SysctrlPclkDiv8);
            AppAdcVirAvgCodeGet(&u32VirAdcCode);            ///< 表面温度 ADC采样
            AppAdcNtcHAvgCodeGet(&u32NtcHAdcCode);          ///< 环境温度RH ADC采样
            AppAdcNtcLAvgCodeGet(&u32NtcLAdcCode);          ///< 表面温度RL ADC采样
            Sysctrl_SetPCLKDiv(SysctrlPclkDiv1);

            u32VirAdcCodeAcc  += u32VirAdcCode;
            u32NtcHAdcCodeAcc += u32NtcHAdcCode;
            u32NtcLAdcCodeAcc += u32NtcLAdcCode;
        }

        u32VirAdcCode  = (u32VirAdcCodeAcc  + 0x8u)>>4u;   ///< 表面温度 ADC CODE    
        u32NtcHAdcCode = (u32NtcHAdcCodeAcc + 0x8u)>>4u;   ///< 环境温度RH ADC CODE
        u32NtcLAdcCode = (u32NtcLAdcCodeAcc + 0x8u)>>4u;   ///< 表面温度RL ADC CODE

        __enable_irq();
    }

    Gpio_ClrIO(M_ADC_VBIRS_PORT, M_ADC_VBIRS_PIN);

    *uViR = u32VirAdcCode;
    *uVNtcH = u32NtcHAdcCode;
    *uVNtcL = u32NtcLAdcCode;

    return TRUE;
}

///< ADC 采样及温度计算
// TRUE  - 标准模式(使用标定后的值)
// FALSE - 标定(测试)模式 
void AppTempCalculate(CalData_t *pCal)
{
    static int i = 0;
    uint32_t  u32SampIndex;     ///< 采样次数
    uint32_t  uViR, uVNtcH, uVNtcL;     ///< ADC 采样值
    float32_t fNtcTemp, fBlackTemp, fSurfaceTemp, fHumanTemp;

    ASSERT(pCal);

    if(FALSE == AppAdcCodeGet(&uViR, &uVNtcH, &uVNtcL)) {
        return;
    }

    ///< 环境温度获取
    fNtcTemp = NNA_NtcTempGet(uVNtcH, uVNtcL);       ///< NTC 环境温度值获取

    ///< 黑体/物体 表面温度
    fBlackTemp = NNA_SurfaceTempGet(pCal, fNtcTemp, uViR, 1.0);

    ///< 物体表面 
    fSurfaceTemp = NNA_SurfaceTempGet(pCal, fNtcTemp, uViR, 0.98295);

    ///< 人体温度
    fHumanTemp = NNA_HumanBodyTempGet(pCal, fSurfaceTemp);

    DBG_PRINT("ViR: %u Ntc: %2.2f Black: %2.2f Surf: %2.2f Hum: %2.2f\r\n", \
                    uViR, fNtcTemp, fBlackTemp, fSurfaceTemp, fHumanTemp);
}

///< 校准(标定)模式API
static void AppCalibration(void)
{
	CalData_t Cal;
    float32_t fNtc;
    uint8_t u8CaType = 0;
    uint32_t uNtcH, uNtcL, uViR;

    memset(&Cal, 0, sizeof(Cal));

    ///< 等按键触发
    while(1) {
        if(!KEY_TRIG()) {
            if(u8CaType == 0) {
                AppLedEnable(1);
                delay1ms(200);
            } else {
                AppLedEnable(1);
                delay1ms(500);
            }
            continue;
        }
        ///< 清除按键
        KEY_CLR_TRIG();

        ///< 读取ADC
        if(!AppAdcCodeGet(&uViR, &uNtcH, &uNtcL)) {
            delay1ms(100);
            continue;
        }

        ///< 环境温度
        fNtc = NNA_NtcTempGet(uNtcH, uNtcL);

        if(u8CaType == 0) {
            NNA_Calibration(&Cal, fNtc, 37, uViR);
            AppBeepBlink((SystemCoreClock/1500));
        } else {
            NNA_Calibration(&Cal, fNtc, 42, uViR);
            AppBeepBlink((SystemCoreClock/1500));
            /// finished
            break;
        }
        u8CaType ++;
    }

    ///< 区间补偿常数
    Cal.u8FixHuman = 29;

    ///< 回写校准数据
    AppCalStore(&Cal, sizeof(Cal));
}

/**
 ******************************************************************************
 ** \brief  Main function of project
 **
 ** \return uint32_t return value, if needed
 **
 ** This sample
 **
 ******************************************************************************/
int32_t main(void)
{
    ///< 系统初始化
    App_SystemInit();

    ///< 可用于简单采样测试(需要开启UART及其端口配置)            
    // DBG_PRINT("Temp Test >>> \r\n");

    AppLedEnable(LedLightBlue);
    AppLcdDisplayAll();
    // AppLcdBlink();          ///< 初次上电开机LCD全屏显示闪烁两次

    // Gpio_SetIO(M_ADC_VBIRS_PORT, M_ADC_VBIRS_PIN); //Always ON

    while(1)
    {
        CalData_t *pCal;
        //for test lcd
        #if 0
        {   
            extern void AppLcdDebug(void);
            AppLedEnable(LedLightBlue);
            delay1ms(500);
            
            AppLcdDebug();
            delay1ms(1000);
        }
        #endif

        while((pCal = AppCalGet()) == NULL) {
            AppCalibration();
            delay1ms(100);
        }

        ///<*** 温度数据采集及转换
        if(KEY_TRIG()) {
            KEY_CLR_TRIG();
            AppTempCalculate(pCal);
            AppBeepBlink((SystemCoreClock/1500));
            // AppLcdBlink();
        }

        if(KEY_LEFT()) {
            KEY_CLR_LEFT();
            AppCalClean();
            AppLcdBlink();
            AppBeepBlink((SystemCoreClock/1500));
        }

        delay1ms(100);
    }
}
   

///< LVD 中断服务函数
void Lvd_IRQHandler(void)
{
    uint8_t u8Index;
    Lvd_ClearIrq();

    u8Index = 5;
    while(u8Index--)
    {
        
        AppLcdDisplayUpdate();
        delay1ms(300);
        
        AppLcdDisplayUpdate();
        delay1ms(300);
    }
    
    ///<*** 配置IO,准备进入超低功耗模式
    AppLcdClearAll();
    ///< 状态切换至关机模式
    Adc_Disable();
    Bgr_BgrDisable();
    Lpm_GotoDeepSleep(FALSE);    

}


///< GPIO 中断服务程序 ———— 测温及选择功能键
void PortC_IRQHandler(void)
{
    delay1ms(10);
    if (TRUE == Gpio_GetIrqStatus(M_KEY_LEFT_PORT, M_KEY_LEFT_PIN))
    {
        Gpio_ClearIrq(M_KEY_LEFT_PORT, M_KEY_LEFT_PIN);
        if(FALSE == Gpio_GetInputIO(M_KEY_LEFT_PORT, M_KEY_LEFT_PIN))
        {            
            //标定按键按下
            KEY_SET_LEFT();
        //……    
            //重新标定自动关机时间
            AppRtcFeed();
            Rtc_Cmd(TRUE);
            //……
        }
        return;
    }
    
    if (TRUE == Gpio_GetIrqStatus(M_KEY_MID_PORT, M_KEY_MID_PIN))
    {
        Gpio_ClearIrq(M_KEY_MID_PORT, M_KEY_MID_PIN);
        if(FALSE == Gpio_GetInputIO(M_KEY_MID_PORT, M_KEY_MID_PIN))
        {
            //标定按键按下
            KEY_SET_MID();
            //……
            //重新标定自动关机时间
            AppRtcFeed();
            Rtc_Cmd(TRUE);
        }
        return;
    }
    
    if (TRUE == Gpio_GetIrqStatus(M_KEY_RIGHT_PORT, M_KEY_RIGHT_PIN))
    {
        Gpio_ClearIrq(M_KEY_RIGHT_PORT, M_KEY_RIGHT_PIN);
        if(FALSE == Gpio_GetInputIO(M_KEY_RIGHT_PORT, M_KEY_RIGHT_PIN))
        {
            //标定按键按下
            KEY_SET_RIGHT();
            //……
            //重新标定自动关机时间
            AppRtcFeed();
            Rtc_Cmd(TRUE);
        }
        return;
    }
}

///< GPIO 中断服务程序 ———— 模式功能键
void PortD_IRQHandler(void)
{
    delay1ms(100);
    if (TRUE == Gpio_GetIrqStatus(M_KEY_TRIG_PORT, M_KEY_TRIG_PIN))
    {
        Gpio_ClearIrq(M_KEY_TRIG_PORT, M_KEY_TRIG_PIN);
        if(FALSE == Gpio_GetInputIO(M_KEY_TRIG_PORT, M_KEY_TRIG_PIN))
        {            
            //标定按键按下
            KEY_SET_TRIG();
            
            //重新标定自动关机时间
            AppRtcFeed();
            Rtc_Cmd(TRUE);
            
            //……
        }
        return;
    }
}


void Rtc_IRQHandler(void)
{
    if (Rtc_GetAlmfItStatus() == TRUE) //闹铃中断
    {
        Rtc_ClearAlmfItStatus();       //清中断标志位
        //重新标定自动关机时间
        AppRtcFeed();//重新标定自动关机时间
        Rtc_Cmd(FALSE);
        
        ///<*** 配置IO,关闭LCD，准备进入超低功耗模式
        AppLcdClearAll();
        
        ///< 状态切换至关机模式
        enMState = PowerOnMode;
        //Lpm_GotoDeepSleep(FALSE);
        
    }
}

/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/


