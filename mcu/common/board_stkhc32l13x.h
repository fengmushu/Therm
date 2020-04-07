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
/** \file stkhc32l13x.h
 **
 ** stk board common define.
 ** @link SampleGroup Some description @endlink
 **
 **   - 2018-03-09  1.0  Lux First version.
 **
 ******************************************************************************/
#ifndef __BOARD_STKHC32L13X_H__
#define __BOARD_STKHC32L13X_H__

///< XTH
#define SYSTEM_XTH          (32*1000*1000u)     ///< 32MHz

///< XTL
#define SYSTEM_XTL          (32768u)            ///< 32768Hz

///< M GPIO KEYS
#define M_KEY_LEFT_PORT       GpioPortC
#define M_KEY_LEFT_PIN        GpioPin10
#define M_KEY_MID_PORT       GpioPortC
#define M_KEY_MID_PIN        GpioPin11
#define M_KEY_RIGHT_PORT       GpioPortC
#define M_KEY_RIGHT_PIN        GpioPin12
#define M_KEY_TRIG_PORT       GpioPortD
#define M_KEY_TRIG_PIN        GpioPin6
#define M_KEY_SWITCH_PORT       GpioPortD
#define M_KEY_SWITCH_PIN        GpioPin2

///< M ADC EXREF
// #define M_ADC_EXREF_PORT  GpioPortB
// #define M_ADC_EXREF_PIN   GpioPin1

///< M ADC CHx
// V-OUT
#define M_ADC_VOUT_PORT    GpioPortA
#define M_ADC_VOUT_PIN     GpioPin0
// NTC+
#define M_ADC_NTCH_PORT  GpioPortA
#define M_ADC_NTCH_PIN   GpioPin1
// NTC-
#define M_ADC_NTCL_PORT  GpioPortA
#define M_ADC_NTCL_PIN   GpioPin2
// VIN +/- --- V2.0硬件, 已删除
// V-Bias-REF-IN
// #define M_ADC_VREF_PORT  GpioPortA
// #define M_ADC_VREF_PIN   GpioPin3
// V-Bias-ON/OFF
#define M_ADC_VBIRS_PORT  GpioPortA
#define M_ADC_VBIRS_PIN   GpioPin15

///< M LCD SEG/COM
#define M_LCD_COM0_PORT   GpioPortA
#define M_LCD_COM0_PIN    GpioPin9
#define M_LCD_COM1_PORT   GpioPortA
#define M_LCD_COM1_PIN    GpioPin10
#define M_LCD_COM2_PORT   GpioPortA
#define M_LCD_COM2_PIN    GpioPin11
#define M_LCD_COM3_PORT   GpioPortA
#define M_LCD_COM3_PIN    GpioPin12
//< M LCD SEG0-19
#define M_LCD_SEG0_PORT   GpioPortA
#define M_LCD_SEG0_PIN    GpioPin8 
#define M_LCD_SEG1_PORT   GpioPortC
#define M_LCD_SEG1_PIN    GpioPin9 
#define M_LCD_SEG2_PORT   GpioPortC
#define M_LCD_SEG2_PIN    GpioPin8 
#define M_LCD_SEG3_PORT   GpioPortC
#define M_LCD_SEG3_PIN    GpioPin7 
#define M_LCD_SEG4_PORT   GpioPortC
#define M_LCD_SEG4_PIN    GpioPin6 
#define M_LCD_SEG5_PORT   GpioPortB
#define M_LCD_SEG5_PIN    GpioPin15
#define M_LCD_SEG6_PORT   GpioPortB
#define M_LCD_SEG6_PIN    GpioPin14
#define M_LCD_SEG7_PORT   GpioPortB
#define M_LCD_SEG7_PIN    GpioPin13
#define M_LCD_SEG8_PORT   GpioPortB
#define M_LCD_SEG8_PIN    GpioPin12
#define M_LCD_SEG9_PORT   GpioPortB
#define M_LCD_SEG9_PIN    GpioPin11
#define M_LCD_SEG10_PORT   GpioPortB
#define M_LCD_SEG10_PIN    GpioPin10
#define M_LCD_SEG11_PORT   GpioPortB
#define M_LCD_SEG11_PIN    GpioPin2
#define M_LCD_SEG12_PORT   GpioPortB
#define M_LCD_SEG12_PIN    GpioPin1
#define M_LCD_SEG13_PORT   GpioPortB
#define M_LCD_SEG13_PIN    GpioPin0
#define M_LCD_SEG14_PORT   GpioPortC
#define M_LCD_SEG14_PIN    GpioPin5
#define M_LCD_SEG15_PORT   GpioPortC
#define M_LCD_SEG15_PIN    GpioPin4
#define M_LCD_SEG16_PORT   GpioPortA
#define M_LCD_SEG16_PIN    GpioPin7
#define M_LCD_SEG17_PORT   GpioPortA
#define M_LCD_SEG17_PIN    GpioPin6
#define M_LCD_SEG18_PORT   GpioPortA
#define M_LCD_SEG18_PIN    GpioPin5
#define M_LCD_SEG19_PORT   GpioPortA
#define M_LCD_SEG19_PIN    GpioPin4

//< M BEEP
#define M_BEEP_PORT       GpioPortD
#define M_BEEP_PIN        GpioPin7

///< M LED
#define M_LED1_PORT       GpioPortC
#define M_LED1_PIN        GpioPin13
#define M_LED2_PORT       GpioPortC
#define M_LED2_PIN        GpioPin14
// #define M_LED3_PORT       GpioPortC
// #define M_LED3_PIN        GpioPin15

///< M I2C EEPROM
#define M_E2_I2C0_SCL_PORT       GpioPortB
#define M_E2_I2C0_SCL_PIN        GpioPin8
#define M_E2_I2C0_SDA_PORT       GpioPortB
#define M_E2_I2C0_SDA_PIN        GpioPin9

// DBG_CONSOLE
#define DBG_CONSOLE         M0P_UART1
#define DBG_CONSOLE_GPIO    GpioPortD
#define DBG_CONSOLE_TX      GpioPin0
#define DBG_CONSOLE_RX      GpioPin1

#endif