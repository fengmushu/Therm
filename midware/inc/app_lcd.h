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

/*******************************************************************************
 ** \brief lcd相关函数声明
 note: 
 AppLcdSetXXX函数，调用后需调用AppLcdDisplayUpdate更新显示屏。
 ******************************************************************************/
extern void sys_display_init(void);

extern void AppLcdDisplayAll(void);

// say a delay ms to allow lcd stay its content on demand
extern void AppLcdDisplayUpdate(uint32_t delay_ms);

extern void AppLcdClearAll(void);

void AppLcdEnable(void);

void AppLcdDisable(void);

#ifdef __cplusplus
#endif

#endif /* __APP_LCD_H__ */
/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/


