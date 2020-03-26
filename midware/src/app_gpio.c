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


//串口引脚配置
void _AppUartPortInit(void)
{
    stc_gpio_cfg_t stcGpioCfg;
    
    DDL_ZERO_STRUCT(stcGpioCfg);
    
    stcGpioCfg.enDir = GpioDirOut;
    Gpio_Init(GpioPortA,GpioPin9,&stcGpioCfg);
    Gpio_SetAfMode(GpioPortA,GpioPin9,GpioAf1); //配置PA09 为UART0 TX
    stcGpioCfg.enDir = GpioDirIn;
    Gpio_Init(GpioPortA,GpioPin10,&stcGpioCfg);
    Gpio_SetAfMode(GpioPortA,GpioPin10,GpioAf1);//配置PA10 为UART0 RX
}


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
    
    Gpio_Init(M_KEY_USER0_PORT, M_KEY_USER0_PIN, &stcGpioCfg);
    Gpio_Init(M_KEY_USER1_PORT, M_KEY_USER1_PIN, &stcGpioCfg);
    Gpio_Init(M_KEY_USER2_PORT, M_KEY_USER2_PIN, &stcGpioCfg);
    Gpio_Init(M_KEY_USER3_PORT, M_KEY_USER3_PIN, &stcGpioCfg);
    
    Gpio_EnableIrq(M_KEY_USER0_PORT, M_KEY_USER0_PIN, GpioIrqFalling);
    Gpio_EnableIrq(M_KEY_USER1_PORT, M_KEY_USER1_PIN, GpioIrqFalling);
    Gpio_EnableIrq(M_KEY_USER2_PORT, M_KEY_USER2_PIN, GpioIrqFalling);
    Gpio_EnableIrq(M_KEY_USER3_PORT, M_KEY_USER3_PIN, GpioIrqFalling);
    
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
    
    Gpio_SetIO(M_LED1_PORT, M_LED1_PIN);
    Gpio_SetIO(M_LED2_PORT, M_LED2_PIN);
    Gpio_SetIO(M_LED3_PORT, M_LED3_PIN);
    
    Gpio_Init(M_LED1_PORT, M_LED1_PIN, &stcGpioCfg);
    Gpio_Init(M_LED2_PORT, M_LED2_PIN, &stcGpioCfg);
    Gpio_Init(M_LED3_PORT, M_LED3_PIN, &stcGpioCfg);
    
}

///< ADC SENSOR 通信接口初始化
void _AppAdcSensorPortInit(void)
{    
    stc_gpio_cfg_t  stcGpioCfg;
    
    stcGpioCfg.enCtrlMode = GpioAHB;
    stcGpioCfg.enDir      = GpioDirOut;
    stcGpioCfg.enDrv      = GpioDrvH;
    stcGpioCfg.enOD       = GpioOdDisable;
    stcGpioCfg.enPd       = GpioPdDisable;
    stcGpioCfg.enPu       = GpioPuEnable;
    
    Gpio_SetIO(M_S_CS_PORT,  M_S_CS_PIN) ;
    Gpio_SetIO(M_S_CLK_PORT, M_S_CLK_PIN);
    Gpio_SetIO(M_S_SDO_PORT, M_S_SDO_PIN);
    
    Gpio_Init(M_S_CS_PORT,  M_S_CS_PIN,  &stcGpioCfg);
    Gpio_Init(M_S_CLK_PORT, M_S_CLK_PIN, &stcGpioCfg);
    Gpio_Init(M_S_SDO_PORT, M_S_SDO_PIN, &stcGpioCfg);
    
}

///< BEEP 初始化
void _AppBeepPortInit(void)
{    
    stc_gpio_cfg_t  stcGpioCfg;
    
    stcGpioCfg.enCtrlMode = GpioAHB;
    stcGpioCfg.enDir      = GpioDirOut;
    stcGpioCfg.enDrv      = GpioDrvH;
    stcGpioCfg.enOD       = GpioOdDisable;
    stcGpioCfg.enPd       = GpioPdDisable;
    stcGpioCfg.enPu       = GpioPuDisable;
    
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
    Gpio_SetAnalogMode(M_ADC_AIN00_PORT, M_ADC_AIN00_PIN);        //PA01
    Gpio_SetAnalogMode(M_ADC_AIN01_PORT, M_ADC_AIN01_PIN);        //PA02
    Gpio_SetAnalogMode(M_ADC_AIN03_PORT, M_ADC_AIN03_PIN);        //PA03
    Gpio_SetAnalogMode(M_ADC_VIR_PORT,   M_ADC_VIR_PIN);          //PA00
    
    Gpio_SetAnalogMode(GpioPortB, GpioPin1);        //PB01 (ADC_VREF)
}

///< LCD 端口初始化
void _AppLcdPortInit(void)
{
    Gpio_SetAnalogMode(M_LCD_COM0_PORT, M_LCD_COM0_PIN); //COM0
    Gpio_SetAnalogMode(M_LCD_COM1_PORT, M_LCD_COM1_PIN); //COM1
    Gpio_SetAnalogMode(M_LCD_COM2_PORT, M_LCD_COM2_PIN); //COM2
    Gpio_SetAnalogMode(M_LCD_COM3_PORT, M_LCD_COM3_PIN); //COM3   

    Gpio_SetAnalogMode(M_LCD_SEG0_PORT, M_LCD_SEG0_PIN); //SEG0
    Gpio_SetAnalogMode(M_LCD_SEG1_PORT, M_LCD_SEG1_PIN); //SEG1
    Gpio_SetAnalogMode(M_LCD_SEG2_PORT, M_LCD_SEG2_PIN); //SEG2
    Gpio_SetAnalogMode(M_LCD_SEG3_PORT, M_LCD_SEG3_PIN); //SEG3
    Gpio_SetAnalogMode(M_LCD_SEG4_PORT, M_LCD_SEG4_PIN); //SEG4
    Gpio_SetAnalogMode(M_LCD_SEG5_PORT, M_LCD_SEG5_PIN); //SEG5
    Gpio_SetAnalogMode(M_LCD_SEG6_PORT, M_LCD_SEG6_PIN); //SEG6
    Gpio_SetAnalogMode(M_LCD_SEG7_PORT, M_LCD_SEG7_PIN); //SEG7
    Gpio_SetAnalogMode(M_LCD_SEG8_PORT, M_LCD_SEG8_PIN); //SEG8
    Gpio_SetAnalogMode(M_LCD_SEG9_PORT, M_LCD_SEG9_PIN); //SEG9
}

void AppMGpioInit(void)
{
    ///< 开启GPIO外设时钟
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);
    
    ///< KEY 按键初始化
    _AppKeyPortInit();
    
    ///< LED 背光灯初始化
    _AppLedPortInit();  
    
    ///< ADC SENSOR 通信接口初始化
    _AppAdcSensorPortInit(); 
    
    ///< LCD 端口初始化
    _AppLcdPortInit();
    
    ///< BEEP 初始化
    _AppBeepPortInit();
    
    ///< EEPROM I2C 端口初始化
    _AppEeI2cPortInit();
    
    ///< ADC 采样端口初始化
    _AppAdcPortInit();
    
    ///< UART 端口初始化
    _AppUartPortInit();
    
}

//@} // GpioGroup


/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

