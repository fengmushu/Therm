/******************************************************************************
* Copyright (C) 2019, Huada Semiconductor Co.,Ltd All rights reserved.
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
/** \file app_lcd.h
 **
 ** Header file for lcd Converter functions
 ** @link LCD Group Some description @endlink
 **
 **   - 2019-04-02      First Version
 **
 ******************************************************************************/
#ifndef __APP_LCD_H__
#define __APP_LCD_H__
/******************************************************************************/
/* Include files                                                              */
/******************************************************************************/
#include "ddl.h"

/******************************************************************************
 ** Global type definitions
 *****************************************************************************/
#define APP_LCD_ENABLE()     {M0P_LCD->CR0_f.EN = TRUE;}
#define APP_LCD_DISABLE()    {M0P_LCD->CR0_f.EN = TRUE;}

 /**
 ******************************************************************************
 ** \brief 温度类型
 *****************************************************************************/
typedef enum enTempMode
{
    Celsius     = 0,            /*!< 摄氏度 */
    Fahrenheit  = 1,            /*!< 华氏度 */
    TempNone    = 2,            /*!< 无显示 */
}enTempMode_t;

 /**
 ******************************************************************************
 ** \brief 检测类型
 *****************************************************************************/
typedef enum enCheckMode
{
    Body        = 0,            /*!< body */
    Surface     = 1,            /*!< surface temp */
    CheckNone   = 2,            /*!< 无显示 */
}enCheckMode_t;

/**
******************************************************************************
** \brief 字符串显示格式
*****************************************************************************/
typedef enum enStrType
{
    Str_F1      = 0,            /* F1 */
    Str_F2      = 1,            /* F2 */
    Str_F3      = 2,            /* F3 */
    Str_F4      = 3,            /* F4 */
    Str_ON      = 4,            /* oN */
    Str_OFF     = 5,            /* oFF */
    Str_LO      = 6,            /* Lo */
    Str_HI      = 7,            /* HI */
    Str_NONE    = 8,            /* 不显示 */
    Str_MAX
}enStrType_t;



/*******************************************************************************
 ** \brief lcd相关函数声明
 note: 
 AppLcdSetXXX函数，调用后需调用AppLcdDisplayUpdate更新显示屏。
 ******************************************************************************/
extern void AppLcdInit(void);

extern void AppLcdDisplayClear(void);

extern void AppLcdDisplayAll(void);

extern void AppLcdSetLock(boolean_t display);

extern void AppLcdSetBuzzer(boolean_t display);

extern void AppLcdSetBattery(boolean_t display);

extern void AppLcdSetCheckMode(enCheckMode_t CheckMode, boolean_t display);

extern void AppLcdSetTempMode(enTempMode_t TempMode,  boolean_t display);

extern void AppLcdSetTemp(uint16_t Temp);

extern void AppLcdClearTemp(void);

extern void AppLcdSetLogTemp(uint16_t Temp, uint16_t Index);

extern void AppLcdClearLogTemp(void);

extern void AppLcdSetLogIndex(uint8_t icon, uint16_t index);

extern void AppLcdSetLogRawNumber(int16_t Temp, 
    boolean_t dis_dot, uint8_t min_digits);

extern void AppLcdSetString(enStrType_t StrType);

extern void AppLcdSetRawNumber(int16_t Temp, boolean_t dis_dot, uint8_t min_digits);

// say a delay ms to allow lcd stay its content on demand
extern void AppLcdDisplayUpdate(uint32_t delay_ms);

extern void AppLcdClearAll(void);

extern void AppLcdBlink(void); 

void AppLcdEnable(void);

void AppLcdDisable(void);

#ifdef __cplusplus
#endif

#endif /* __APP_LCD_H__ */
/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/


