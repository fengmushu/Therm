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
#include "app_cal.h"
#include "app_key.h"
#include "app_i2c.h"
#include "app_data.h"
#include "app_timer.h"
#include "fsm.h"


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
#define KEY_CLR_ALL()      (gu32UserKeyFlag = 0)

/******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

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
    Sysctrl_SetPeripheralGate(SysctrlPeripheralRtc, TRUE);  //RTC模块时钟打开
    Sysctrl_SetPeripheralGate(SysctrlPeripheralCrc, TRUE);  //需要校验和时钟
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
    AppRtcInit();

#ifdef DEBUG
    ///< 串口初始化
    AppUartInit();
#endif

    ///< 参数调整区初始化0
    AppParaAreaInit();
    
    ///< 唤醒相关
    AppPmuInit();
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

    app_i2c_init();

    // hold [FN] on PWRON to force calibration mode
    if (key_pressed_query(KEY_FN)) {
        AppCalibration();
    } else {
        // if no cal data stored, goto calibration
        if (AppCalLoad() == NULL) {
            AppCalibration();
        }
    }

    timer3_init();
    app_runtime_init(&g_runtime);

    fsm_init(&g_fsm);
    fsm_start(&g_fsm);

    // dead loop inside until fsm exits (will not, though)
    fsm_process(&g_fsm);

    fsm_shutdown(&g_fsm, FSM_STATE_FATAL);
}

void Lvd_IRQHandler(void)
{
    Lvd_ClearIrq();

    g_rt->battery_low = 1;
}

void Rtc_IRQHandler(void)
{
    if (Rtc_GetPridItStatus() == TRUE) //闹铃中断
    {
        Rtc_ClearPrdfItStatus();       //清中断标志位
        AppRtcUpdate();
    }
}

/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/


