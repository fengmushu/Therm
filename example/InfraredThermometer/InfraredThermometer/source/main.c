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

#define KEY_LEFT()     (gu32UserKeyFlag & (1u < Key_Left))
#define KEY_MID()      (gu32UserKeyFlag & (1u < Key_Mid))
#define KEY_RIGHT()    (gu32UserKeyFlag & (1u < Key_Right))
#define KEY_TRIG()     (gu32UserKeyFlag & (1u < Key_Trig))
#define KEY_SWITCH()   (gu32UserKeyFlag & (1u < Key_Switch))

#define KEY_SET_LEFT()     (gu32UserKeyFlag |= (1u < Key_Left))
#define KEY_SET_MID()      (gu32UserKeyFlag |= (1u < Key_Mid))
#define KEY_SET_RIGHT()    (gu32UserKeyFlag |= (1u < Key_Right))
#define KEY_SET_TRIG()     (gu32UserKeyFlag |= (1u < Key_Trig))
#define KEY_SET_SWITCH()   (gu32UserKeyFlag |= (1u < Key_Switch))

#define KEY_CLR_LEFT()     (gu32UserKeyFlag &= ~(1u < Key_Left))
#define KEY_CLR_MID()      (gu32UserKeyFlag &= ~(1u < Key_Mid))
#define KEY_CLR_RIGHT()    (gu32UserKeyFlag &= ~(1u < Key_Right))
#define KEY_CLR_TRIG()     (gu32UserKeyFlag &= ~(1u < Key_Trig))
#define KEY_CLR_SWITCH()   (gu32UserKeyFlag &= ~(1u < Key_Switch))

/******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
static volatile uint32_t gu32AdcNtcResult = 0, gu32AdcVirResult = 0;
static volatile uint32_t gu32UserKeyFlag = USERKEYFALSE;
// volatile uint32_t gVolFlag   = CHARGEFULL;
static volatile enMState_t enMState = PowerOnMode;

static volatile int enCalType = 1;

/******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
///< 环境温度、黑体温度、人体温度
static float32_t gf32NtcTemp, gf32BlackBodyTemp, gf32HumanBodyTemp;
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


///< ADC 采样及温度计算
// TRUE  - 标准模式(使用标定后的值)
// FALSE - 标定(测试)模式 
void AppAdcColTemp(boolean_t bMarkEn)
{
    static int i = 0;
    uint32_t  u32SampIndex;     ///< 采样次数
    uint32_t  u32VirAdcCode, u32NtcHAdcCode, u32NtcLAdcCode;     ///< ADC 采样值
    uint32_t  u32VirAdcCodeAcc, u32NtcHAdcCodeAcc, u32NtcLAdcCodeAcc;       ///< ADC 累加值

    Gpio_SetIO(M_ADC_VBIRS_PORT, M_ADC_VBIRS_PIN);
    delay1ms(200);
    
    ///<*** ADC数据采集     
    {
        __disable_irq();

        u32SampIndex      = 0x10u;
        u32VirAdcCodeAcc  = 0;
        u32NtcHAdcCodeAcc = 0;
        u32NtcLAdcCodeAcc = 0;

        while(u32SampIndex--)
        {
            // if(u32SampIndex&0x8u)
            // {
            //     gstcLcdDisplayCfg.u16Num = LCDCHAR__;  
            //     AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg));
            // }
            // else
            // {
            //     gstcLcdDisplayCfg.u16Num = 0xFFFEu;
            //     AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg));
            // }
            
            APP_LCD_DISABLE();
            Sysctrl_SetPCLKDiv(SysctrlPclkDiv8);
            AppAdcVirAvgCodeGet(&u32VirAdcCode);            ///< 表面温度 ADC采样
            AppAdcNtcHAvgCodeGet(&u32NtcHAdcCode);          ///< 环境温度RH ADC采样
            AppAdcNtcLAvgCodeGet(&u32NtcLAdcCode);          ///< 表面温度RL ADC采样
            Sysctrl_SetPCLKDiv(SysctrlPclkDiv1);
            APP_LCD_ENABLE();
            
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

    ///< 环境温度获取
    gf32NtcTemp = NNA_NtcTempGet(u32NtcHAdcCode, u32NtcLAdcCode);       ///< NTC 环境温度值获取

    ///< 校准模式
    if(bMarkEn) {
        if(enCalType % 2) {
            NNA_Calibration(gf32NtcTemp, 37, u32VirAdcCode);
        } else {
            NNA_Calibration(gf32NtcTemp, 42, u32VirAdcCode);
        }
        enCalType ++;
    }

    ///< 黑体温度获取
    gf32BlackBodyTemp = NNA_BlackBodyTempGet(gf32NtcTemp, u32VirAdcCode, bMarkEn);     ///< VIR 黑体温度值获取

    ///< 人体温度获取
    // gf32HumanBodyTemp = NNA_HumanBodyTempGet(gf32BlackBodyTemp, gf32NtcTemp);        ///< 人体温度值获取

    // DBG_PRINT("NTCH: %u, NTCL: %u\r\n", u32NtcHAdcCode, u32NtcLAdcCode);
    DBG_PRINT("VIR: %u, Ntc: %2.2fC, %2.2fC\r\n", u32VirAdcCode, gf32NtcTemp, gf32BlackBodyTemp);
    // DBG_PRINT("Ntc = %2.1fC, Black = %2.1fC, Human = %2.1fC\r\n", gf32NtcTemp, gf32BlackBodyTemp, gf32HumanBodyTemp);
}


///< 校准(标定)模式API
void App_CalibrationMode(void)
{ 
    // if(TRUE  == Gpio_GetInputIO(M_KEY_MID_PORT, M_KEY_MID_PIN) &&       ///< VIR Mark
    //    FALSE == Gpio_GetInputIO(M_KEY_RIGHT_PORT, M_KEY_RIGHT_PIN))
    // {
    //     Flash_SectorErase(VIRL_PARA_ADDR);
    //     AppAdcColTemp(FALSE);
    //     AppVirLParaMark(((uint32_t)(gf32BlackBodyTemp*100)));
    //     ///< AppAdcColTemp(TRUE);   ///< 标定后再次采集确认
    // }
    // else if(FALSE == Gpio_GetInputIO(M_KEY_MID_PORT, M_KEY_MID_PIN) &&      ///< NTC Mark
    //         TRUE  == Gpio_GetInputIO(M_KEY_RIGHT_PORT, M_KEY_RIGHT_PIN))
    // {
    //     Flash_SectorErase(VIRH_PARA_ADDR);
    //     AppAdcColTemp(FALSE);
    //     AppVirHParaMark(((uint32_t)(gf32BlackBodyTemp*100)));
    //     ///< AppAdcColTemp(TRUE);   ///< 标定后再次采集确认
    // }
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

#if 0
    while(1)
    {   
        switch(enMState)
        {
            case PowerOnMode:           ///< 上电(开机)模式
            {   
                AppLcdBlink();          ///< 初次上电开机LCD全屏显示闪烁两次
                AppLcdInitialMode();    ///< LCD 初始状态显示

                ///< 校准模式判定
                if(TRUE == Gpio_GetInputIO(M_KEY_MID_PORT, M_KEY_MID_PIN) && 
                   TRUE == Gpio_GetInputIO(M_KEY_RIGHT_PORT, M_KEY_RIGHT_PIN))
                {
                    ///< 状态切换至初始状态模式
                    enMState = InitialMode;
                }
                else
                {
                    gstcLcdDisplayCfg.bM9En     = TRUE;
                    AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg));
                    ///< 状态切换至温度校准模式
                    enMState = CalibrationMode;
                }
            }
            break;
            
            case InitialMode:           ///< 初始状态模式
            {
                if(KEY_TRIG())      ///<*** 按键检测
                {
                    KEY_CLR_TRIG();

                    gstcLcdDisplayCfg.bM6En  = TRUE;
                    gstcLcdDisplayCfg.bM5En  = TRUE;
                    gstcLcdDisplayCfg.bM10En = TRUE;
                    gstcLcdDisplayCfg.bM2En  = TRUE;
                    gstcLcdDisplayCfg.bM8En  = TRUE;
                    gstcLcdDisplayCfg.bM3En  = TRUE;
                    gstcLcdDisplayCfg.bM2En  = FALSE;
                    gstcLcdDisplayCfg.enTmpMode = Celsius;
                    gstcLcdDisplayCfg.u16Num = LCDCHAR__;       ///<*** 显示'__._'(需修改)
                    AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg));
                    
                    ///< 状态切换至测量模式
                    enMState = TempMeasureMode;
                }
                else if(KEY_LEFT())
                {
                    KEY_CLR_LEFT();
                    
                    gstcLcdDisplayCfg.bM6En  = TRUE;
                    gstcLcdDisplayCfg.bM5En  = TRUE;
                    gstcLcdDisplayCfg.bM10En = TRUE;
                    gstcLcdDisplayCfg.bM2En  = TRUE;
                    gstcLcdDisplayCfg.bM8En  = TRUE;
                    gstcLcdDisplayCfg.enTmpMode = Celsius;
                    gstcLcdDisplayCfg.u16Num = gstcLcdDisplayCfg.u16Num - 10;  ///<*** 更新前一次存储温度(需修改)
                    AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg));
                    
                    ///< 状态切换至历史数据查询模式
                    enMState = TempScanMode;
                }
            }
                
            break;
            
            case TempScanMode:          ///< 历史数据查询模式
            {
                {
                    if(KEY_LEFT())       ///<*** F1 按键检测
                    {
                        KEY_CLR_LEFT();
                        
                        gstcLcdDisplayCfg.bM6En  = TRUE;
                        gstcLcdDisplayCfg.bM5En  = TRUE;
                        gstcLcdDisplayCfg.bM10En = TRUE;
                        gstcLcdDisplayCfg.bM2En  = TRUE;
                        gstcLcdDisplayCfg.bM8En  = TRUE;
                        gstcLcdDisplayCfg.enTmpMode = Celsius;
                        gstcLcdDisplayCfg.u16Num = gstcLcdDisplayCfg.u16Num - 10;  ///<*** 更新前一次存储温度(需修改)
                        AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg));                        
                        
                    }
                    else if(KEY_TRIG())  ///<*** F2 按键检测
                    {
                        KEY_CLR_TRIG();                        
                    
                        gstcLcdDisplayCfg.bM6En  = TRUE;
                        gstcLcdDisplayCfg.bM5En  = TRUE;
                        gstcLcdDisplayCfg.bM10En = TRUE;
                        gstcLcdDisplayCfg.bM2En  = TRUE;
                        gstcLcdDisplayCfg.bM8En  = TRUE;
                        gstcLcdDisplayCfg.bM3En  = TRUE;
                        gstcLcdDisplayCfg.bM2En  = FALSE;
                        gstcLcdDisplayCfg.enTmpMode = Celsius;
                        gstcLcdDisplayCfg.u16Num = LCDCHAR__;       ///<*** 显示'__._'(需修改)
                        AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg)); 

                        ///< 状态切换至测温模式
                        enMState = TempMeasureMode;
                        
                    }
                }
            }
            break;
            
            case TempMeasureMode:       ///< 温度测量模式
            {
                ///<*** 启动测温
                if(KEY_TRIG())  ///<*** F1 按键检测
                {
                    KEY_CLR_TRIG();                        
                
                    ///<*** 温度数据采集处理     
                    AppAdcColTemp(TRUE);
                    
                    gstcLcdDisplayCfg.u16Num = (gf32HumanBodyTemp + 0.05)*10;
                    gstcLcdDisplayCfg.bM7En  = FALSE;
                    gstcLcdDisplayCfg.bM8En  = FALSE;
                    gstcLcdDisplayCfg.bM5En  = TRUE;
                    gstcLcdDisplayCfg.bM9En  = FALSE;
                    AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg));
                    
                    AppLedEnable(LedLightBlue);
                    AppBeepBlink((SystemCoreClock/1500));
                    delay1ms(1000);
                    AppLedDisable();

                }
                else if(KEY_SWITCH())      ///<*** 关机检测
                {
                    KEY_CLR_SWITCH();
                    {
                        ///< 状态切换至关机模式
                        enMState = PowerOffMode;
                    }
                }
            
            }
            
            break;
            
            case PowerOffMode:          ///< 关机模式
                {
                    if((KEY_TRIG()) || (KEY_LEFT()))      ///<*** 开机检测
                    {
                        KEY_CLR_TRIG();
                        KEY_CLR_LEFT();

                        Adc_Enable();
                        Bgr_BgrEnable();
                        
                        {
                            ///< 状态切换至关机模式
                            enMState = PowerOnMode;
                        }
                    }
                    else
                    {                        
                        ///<*** 配置IO,准备进入超低功耗模式
                        AppLcdClearAll();
                        ///< 状态切换至关机模式
                        Adc_Disable();
                        Bgr_BgrDisable();
                        Lpm_GotoDeepSleep(FALSE);
                    }

                }
            break;
            
            case CalibrationMode:       ///< 校准(标定)模式
            {
                if(KEY_TRIG())
                {
                    KEY_CLR_TRIG();
                    
                    App_CalibrationMode();
                    enMState = PowerOnMode;
                }
            }
            break;
            
            default:
                // ……
                enMState = PowerOnMode;
            break;
                
        }       
    }

#else
    ///< 可用于简单采样测试(需要开启UART及其端口配置)            
    // DBG_PRINT("Temp Test >>> \r\n");

    AppLedEnable(LedLightBlue);
    AppLcdDisplayAll();
    AppLcdBlink();          ///< 初次上电开机LCD全屏显示闪烁两次


    while(1)
    {
        //for test lcd
        #if 1
        {   
            extern void AppLcdDebug(void);
            AppLedEnable(LedLightBlue);
            delay1ms(500);
            
            AppLcdDebug();
            delay1ms(1000);
            continue;
            
        }
        #endif
                
    
        if((enCalType % 2)) {
            AppLedEnable(1);
        } else {
            AppLedEnable(0);
        }

        ///<*** 温度数据采集及转换
        if(KEY_TRIG()) {
            KEY_CLR_TRIG();
            AppAdcColTemp(TRUE);
            AppBeepBlink((SystemCoreClock/1500));
            AppLcdBlink();
        }

        // DBG_PRINT("Ntc = %2.1fC\r\n", gf32NtcTemp);
        // delay1ms(100);
        // DBG_PRINT("Black = %2.1fC\r\n", gf32BlackBodyTemp);
        // delay1ms(100);
        // DBG_PRINT("Human = %2.1fC\r\n", gf32HumanBodyTemp);
        // delay1ms(100);
        // DBG_PRINT("\r\n");

        delay1ms(200);
    }

#endif
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
    delay1ms(100);
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


