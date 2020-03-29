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

/******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
volatile uint32_t gu32AdcNtcResult = 0, gu32AdcVirResult = 0;
volatile uint32_t gu32UserKeyFlag[4] = {USERKEYFALSE, USERKEYFALSE, USERKEYFALSE, USERKEYFALSE};
volatile stc_lcd_display_cfg_t gstcLcdDisplayCfg = {0};
volatile uint32_t gVolFlag   = CHARGEFULL;
volatile enMState_t enMState = PowerOnMode;

/******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
///< 环境温度、黑体温度、人体温度
float32_t gf32NtcTemp, gf32BlackBodyTemp, gf32HumanBodyTemp;
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


///< 系统初始化
void App_SystemInit(void)
{
    ///< GPIO 初始化
    AppMGpioInit();
    
    ///< ADC 模块初始化
    AppMAdcInit();
    
    ///< LCD 模块初始化
    AppLcdInit();
    
    ///< 电量监测模块初始化
    AppVolMonitorInit();
    
    ///< 自动关机模块
    // AppPowerOffModuleInit();
    
    ///< 串口初始化
    AppUartInit();
    
    ///< 参数调整区初始化0
    AppParaAreaInit();
    
    // printf("Init Sucessful \r\n");
}


///< LCD 初始化模式显示
void AppLcdInitialMode(void)
{
    gstcLcdDisplayCfg.bM6En     = FALSE;
    gstcLcdDisplayCfg.bM5En     = FALSE;
    gstcLcdDisplayCfg.bM2En     = FALSE;
    gstcLcdDisplayCfg.bM7En     = TRUE;
    gstcLcdDisplayCfg.bM8En     = FALSE;
    gstcLcdDisplayCfg.bM9En     = FALSE;
    gstcLcdDisplayCfg.bM10En    = TRUE;
    gstcLcdDisplayCfg.bM11En    = TRUE;
    gstcLcdDisplayCfg.bM3En     = FALSE;
    gstcLcdDisplayCfg.enTmpMode = Char__;
    gstcLcdDisplayCfg.u16Num    = LCDCHAR__;

    AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg));
}


///< 按键状态清除
void AppUserKeyClearAll(void)
{
    gu32UserKeyFlag[0] = USERKEYFALSE;
    gu32UserKeyFlag[1] = USERKEYFALSE;
    gu32UserKeyFlag[2] = USERKEYFALSE;
    gu32UserKeyFlag[3] = USERKEYFALSE;
}


///< ADC 采样及温度计算
// TRUE  - 标准模式(使用标定后的值)
// FALSE - 标定(测试)模式 
void AppAdcColTemp(boolean_t bMarkEn)
{
    uint32_t  u32SampIndex;     ///< 采样次数
    uint32_t  u32VirAdcCode, u32NtcHAdcCode, u32NtcLAdcCode, u32VirBiasAdcCode;     ///< ADC 采样值
    uint32_t  u32VirAdcCodeAcc, u32NtcHAdcCodeAcc, u32NtcLAdcCodeAcc;       ///< ADC 累加值
    
    ///<*** ADC数据采集     
    {
        __disable_irq();

        u32SampIndex      = 0x10u;
        u32VirAdcCodeAcc  = 0;
        u32NtcHAdcCodeAcc = 0;
        u32NtcLAdcCodeAcc = 0;
      
        while(u32SampIndex--)
        {
            if(u32SampIndex&0x8u)
            {
                gstcLcdDisplayCfg.u16Num = LCDCHAR__;  
                AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg));
            }
            else
            {
                gstcLcdDisplayCfg.u16Num = 0xFFFEu;
                AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg));
            }
            
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
        // AppAdcVBiasAvgCodeGet(&u32VirBiasAdcCode);          ///< 红外偏置电压 ADC CODE
        
        __enable_irq();
    }

    printf("VIR: %u, NTCH: %u, NTCL: %u\r\n",
                u32VirAdcCode, u32NtcHAdcCode, u32NtcLAdcCode);

    ///< 环境温度获取
    // gf32NtcTemp = NNA_NtcTempGet(u32NtcHAdcCode, u32NtcLAdcCode);       ///< NTC 环境温度值获取
    
    ///< 黑体温度获取
    // gf32BlackBodyTemp = NNA_BlackBodyTempGet(gf32NtcTemp, u32VirAdcCode, u32VirBiasAdcCode, bMarkEn);     ///< VIR 黑体温度值获取
    
    ///< 人体温度获取
    // gf32HumanBodyTemp = NNA_HumanBodyTempGet(gf32BlackBodyTemp, gf32NtcTemp);        ///< 人体温度值获取
  
}


///< 校准(标定)模式API
void App_CalibrationMode(void)
{ 
    if(TRUE  == Gpio_GetInputIO(M_KEY_USER1_PORT, M_KEY_USER1_PIN) &&       ///< VIR Mark
       FALSE == Gpio_GetInputIO(M_KEY_USER2_PORT, M_KEY_USER2_PIN))
    {
        Flash_SectorErase(VIRL_PARA_ADDR);
        AppAdcColTemp(FALSE);
        AppVirLParaMark(((uint32_t)(gf32BlackBodyTemp*100)));
        ///< AppAdcColTemp(TRUE);   ///< 标定后再次采集确认
    }
    else if(FALSE == Gpio_GetInputIO(M_KEY_USER1_PORT, M_KEY_USER1_PIN) &&      ///< NTC Mark
            TRUE  == Gpio_GetInputIO(M_KEY_USER2_PORT, M_KEY_USER2_PIN))
    {
        Flash_SectorErase(VIRH_PARA_ADDR);
        AppAdcColTemp(FALSE);
        AppVirHParaMark(((uint32_t)(gf32BlackBodyTemp*100)));
        ///< AppAdcColTemp(TRUE);   ///< 标定后再次采集确认
    }
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
                if(TRUE == Gpio_GetInputIO(M_KEY_USER1_PORT, M_KEY_USER1_PIN) && 
                   TRUE == Gpio_GetInputIO(M_KEY_USER2_PORT, M_KEY_USER2_PIN))
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
                if(USERKEYTRUE == gu32UserKeyFlag[3])      ///<*** 按键检测
                {
                    AppUserKeyClearAll();

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
                else if(USERKEYTRUE == gu32UserKeyFlag[0])
                {
                    AppUserKeyClearAll();
                    
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
                    if(USERKEYTRUE == gu32UserKeyFlag[0])       ///<*** F1 按键检测
                    {
                        AppUserKeyClearAll();
                        
                        gstcLcdDisplayCfg.bM6En  = TRUE;
                        gstcLcdDisplayCfg.bM5En  = TRUE;
                        gstcLcdDisplayCfg.bM10En = TRUE;
                        gstcLcdDisplayCfg.bM2En  = TRUE;
                        gstcLcdDisplayCfg.bM8En  = TRUE;
                        gstcLcdDisplayCfg.enTmpMode = Celsius;
                        gstcLcdDisplayCfg.u16Num = gstcLcdDisplayCfg.u16Num - 10;  ///<*** 更新前一次存储温度(需修改)
                        AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg));                        
                        
                    }
                    else if(USERKEYTRUE == gu32UserKeyFlag[3])  ///<*** F2 按键检测
                    {
                        AppUserKeyClearAll();                        
                    
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
                if(USERKEYTRUE == gu32UserKeyFlag[0])  ///<*** F1 按键检测
                {
                    AppUserKeyClearAll();                        
                
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
                else if(USERKEYTRUE == gu32UserKeyFlag[3])      ///<*** 关机检测
                {
                    AppUserKeyClearAll();

                    {
                        ///< 状态切换至关机模式
                        enMState = PowerOffMode;
                    }
                }
            
            }
            
            break;
            
            case PowerOffMode:          ///< 关机模式
                {
                    if((USERKEYTRUE == gu32UserKeyFlag[3]) || (USERKEYTRUE == gu32UserKeyFlag[0]))      ///<*** 开机检测
                    {
                        AppUserKeyClearAll();

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
                if(USERKEYTRUE == gu32UserKeyFlag[3])
                {
                    AppUserKeyClearAll();
                    
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
    // printf("Temp Test >>> \r\n");

    while(1)
    {
        AppLedEnable(-1);
        AppBeepBlink((SystemCoreClock/1500));

        delay1ms(1500);
        
        ///<*** 温度数据采集及转换     
        AppAdcColTemp(FALSE);
        
        // printf("Ntc = %2.1fC\r\n", gf32NtcTemp);
        // delay1ms(100);
        // printf("Black = %2.1fC\r\n", gf32BlackBodyTemp);
        // delay1ms(100);
        // printf("Human = %2.1fC\r\n", gf32HumanBodyTemp);
        // delay1ms(100);
        // printf("\r\n");
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
        gstcLcdDisplayCfg.bM11En  = FALSE;
        AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg));
        delay1ms(300);
        
        gstcLcdDisplayCfg.bM11En  = TRUE;
        AppLcdDisplayUpdate((stc_lcd_display_cfg_t*)(&gstcLcdDisplayCfg));
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
    if (TRUE == Gpio_GetIrqStatus(M_KEY_USER0_PORT, M_KEY_USER0_PIN))
    {
        Gpio_ClearIrq(M_KEY_USER0_PORT, M_KEY_USER0_PIN);
        if(FALSE == Gpio_GetInputIO(M_KEY_USER0_PORT, M_KEY_USER0_PIN))
        {            
            //标定按键按下
            gu32UserKeyFlag[0] = USERKEYTRUE;
            //……
            //重新标定自动关机时间
            AppRtcFeed();
            Rtc_Cmd(TRUE);
            //……
        }
        return;
    }
    
    if (TRUE == Gpio_GetIrqStatus(M_KEY_USER1_PORT, M_KEY_USER1_PIN))
    {
        Gpio_ClearIrq(M_KEY_USER1_PORT, M_KEY_USER1_PIN);
        if(FALSE == Gpio_GetInputIO(M_KEY_USER1_PORT, M_KEY_USER1_PIN))
        {
            //标定按键按下
            gu32UserKeyFlag[1] = USERKEYTRUE;
            //……
            //重新标定自动关机时间
            AppRtcFeed();
            Rtc_Cmd(TRUE);
        }
        return;
    }
    
    if (TRUE == Gpio_GetIrqStatus(M_KEY_USER2_PORT, M_KEY_USER2_PIN))
    {
        Gpio_ClearIrq(M_KEY_USER2_PORT, M_KEY_USER2_PIN);
        if(FALSE == Gpio_GetInputIO(M_KEY_USER2_PORT, M_KEY_USER2_PIN))
        {
            //标定按键按下
            gu32UserKeyFlag[2] = USERKEYTRUE;
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
    if (TRUE == Gpio_GetIrqStatus(M_KEY_USER3_PORT, M_KEY_USER3_PIN))
    {
        Gpio_ClearIrq(M_KEY_USER3_PORT, M_KEY_USER3_PIN);
        if(FALSE == Gpio_GetInputIO(M_KEY_USER3_PORT, M_KEY_USER3_PIN))
        {            
            //标定按键按下
            gu32UserKeyFlag[3] = USERKEYTRUE;
            
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


