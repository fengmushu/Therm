/******************************************************************************
* Copyright (C) 2018, Huada Semiconductor Co.,Ltd All rights reserved.
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
/** \file Gpio.c
 **
 ** GPIO driver API.
 ** @link Driver Group Some description @endlink
 **
 **   - 2018-04-22  1.0  Lux First version
 **
 ******************************************************************************/

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "app.h"
#include "app_gpio.h"
#include "gpio.h"
#include "sysctrl.h"

/**
 *******************************************************************************
 ** \addtogroup GpioGroup
 ******************************************************************************/
//@{

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define IS_VALID_PIN(port,pin)      (  )
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')         *
 ******************************************************************************/

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/


#ifdef DEBUG
//串口引脚配置
void _AppUartPortInit(void)
{
    stc_gpio_cfg_t stcGpioCfg;
    
    DDL_ZERO_STRUCT(stcGpioCfg);
    stcGpioCfg.enOD       = GpioOdEnable;
    stcGpioCfg.enPd       = GpioPdDisable;
    stcGpioCfg.enPu       = GpioPuDisable;

    stcGpioCfg.enDir = GpioDirOut;
    Gpio_Init(DBG_CONSOLE_GPIO, DBG_CONSOLE_TX,&stcGpioCfg);
    Gpio_SetAfMode(DBG_CONSOLE_GPIO, DBG_CONSOLE_TX, GpioAf3); //配置PD00 为UART1 TX, PA09:1 PB6:2, PD0:3
    stcGpioCfg.enDir = GpioDirIn;
    Gpio_Init(DBG_CONSOLE_GPIO, DBG_CONSOLE_RX, &stcGpioCfg);
    Gpio_SetAfMode(DBG_CONSOLE_GPIO, DBG_CONSOLE_RX, GpioAf3); //配置PD01 为UART1 RX
}
#endif

///< KEY 按键初始化
void _AppKeyPortInit(void)
{    
    stc_gpio_cfg_t  stcGpioCfg;
    
    stcGpioCfg.enCtrlMode = GpioAHB;
    stcGpioCfg.enDir      = GpioDirIn;
    stcGpioCfg.enDrv      = GpioDrvL;
    stcGpioCfg.enOD       = GpioOdDisable;
    stcGpioCfg.enPd       = GpioPdDisable;
    stcGpioCfg.enPu       = GpioPuDisable;
    
    Gpio_Init(M_KEY_TRIG_PORT, M_KEY_TRIG_PIN, &stcGpioCfg);
    Gpio_EnableIrq(M_KEY_TRIG_PORT, M_KEY_TRIG_PIN, GpioIrqFalling);
    Gpio_EnableIrq(M_KEY_TRIG_PORT, M_KEY_TRIG_PIN, GpioIrqRising);

    EnableNvic(PORTC_IRQn, IrqLevel0, TRUE);
    EnableNvic(PORTD_IRQn, IrqLevel0, TRUE);
}

///< LED 背光灯初始化
void _AppLedPortInit(void)
{    
    stc_gpio_cfg_t  stcGpioCfg;
    
    stcGpioCfg.enCtrlMode = GpioAHB;
    stcGpioCfg.enDir      = GpioDirOut;
    stcGpioCfg.enDrv      = GpioDrvH;
    stcGpioCfg.enOD       = GpioOdDisable;
    stcGpioCfg.enPd       = GpioPdDisable;
    stcGpioCfg.enPu       = GpioPuDisable;
    
    Gpio_Init(M_LED_RED_PORT, M_LED_RED_PIN, &stcGpioCfg);
    Gpio_Init(M_LED_GREEN_PORT, M_LED_GREEN_PIN, &stcGpioCfg);

    Gpio_SetIO(M_LED_RED_PORT, M_LED_RED_PIN);
    Gpio_SetIO(M_LED_GREEN_PORT, M_LED_GREEN_PIN);
}

///< BEEP 初始化
void _AppBeepPortInit(void)
{    
    stc_gpio_cfg_t  stcGpioCfg;
    
    stcGpioCfg.enCtrlMode = GpioAHB;
    stcGpioCfg.enDir      = GpioDirOut;
    stcGpioCfg.enDrv      = GpioDrvL;
    stcGpioCfg.enOD       = GpioOdDisable;
    stcGpioCfg.enPd       = GpioPdDisable;
    stcGpioCfg.enPu       = GpioPuEnable;
    
    Gpio_ClrIO(M_BEEP_PORT, M_BEEP_PIN);
    
    Gpio_Init(M_BEEP_PORT, M_BEEP_PIN, &stcGpioCfg);
    
}

///< EEPROM I2C 端口初始化
void _AppEeI2cPortInit(void)
{    
    stc_gpio_cfg_t  stcGpioCfg;
    
    stcGpioCfg.enCtrlMode = GpioAHB;
    stcGpioCfg.enDir      = GpioDirOut;
    stcGpioCfg.enDrv      = GpioDrvH;
    stcGpioCfg.enOD       = GpioOdEnable;
    stcGpioCfg.enPd       = GpioPdDisable;
    stcGpioCfg.enPu       = GpioPuDisable;
    
    Gpio_Init(M_E2_I2C0_SCL_PORT, M_E2_I2C0_SCL_PIN, &stcGpioCfg);
    Gpio_Init(M_E2_I2C0_SDA_PORT, M_E2_I2C0_SDA_PIN, &stcGpioCfg);
    
    Gpio_SetAfMode(M_E2_I2C0_SCL_PORT, M_E2_I2C0_SCL_PIN, GpioAf1);
    Gpio_SetAfMode(M_E2_I2C0_SDA_PORT, M_E2_I2C0_SDA_PIN, GpioAf1);
    
}

///< ADC 采样端口初始化
void _AppAdcPortInit(void)
{
    Gpio_SetAnalogMode(M_ADC_VOUT_PORT, M_ADC_VOUT_PIN);            //PA00

    stc_gpio_cfg_t  stcGpioCfg;

    stcGpioCfg.enCtrlMode = GpioAHB;
    stcGpioCfg.enDir      = GpioDirOut;
    stcGpioCfg.enDrv      = GpioDrvH;
    stcGpioCfg.enOD       = GpioOdEnable;
    stcGpioCfg.enPd       = GpioPdDisable;
    stcGpioCfg.enPu       = GpioPuDisable; //默認開啓

    stcGpioCfg.enOD       = GpioOdDisable;
    Gpio_Init(M_LCD_PWR_EN_PORT, M_LCD_PWR_EN_PIN, &stcGpioCfg);        //PB07 LCD ON/OFF
    Gpio_Init(M_SW_PWR_EN_PORT, M_SW_PWR_EN_PIN, &stcGpioCfg);          //PD05 System ON/OFF

    AppSwPowerOn();
}

///< AFE 计数器输入
void _AppAfePortInit(void)
{
    
}

void AppMGpioInit(void)
{
    ///< 开启GPIO外设时钟
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);
    
    ///< KEY 按键初始化
    _AppKeyPortInit();

    ///< AFE 计数器
    _AppAfePortInit();
    
    ///< LED 背光灯初始化
    _AppLedPortInit();  

    ///< BEEP 初始化
    _AppBeepPortInit();
    
    ///< EEPROM I2C 端口初始化
    _AppEeI2cPortInit();
    
    ///< ADC 采样端口初始化
    _AppAdcPortInit();

#ifdef DEBUG
    ///< UART 端口初始化
    _AppUartPortInit();
#endif
}

//@} // GpioGroup


/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

