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
typedef enum en_tmp_mode_ref
{
    Celsius    = 0x0905u,            /*!< 摄氏度 */
    Fahrenheit = 0x0807u,            /*!< 华氏度 */
    Char__     = 0x0100u,            /*!<   '_'  */
    None       = 0x0000u,            /*!< 无显示 */
}en_tmp_mode_ref_t;


/******************************************************************************
 ** LCD Data Map Config
 *****************************************************************************/
typedef struct stc_lcd_display_cfg
{
    boolean_t bM6En;        ///< 度数标识"°"显示使能 
    boolean_t bM5En;        ///< 人体测试模式使能
    boolean_t bM2En;        ///< 记忆模式使能
    boolean_t bM7En;        ///< 蜂鸣器打开使能
    boolean_t bM8En;        ///< 蜂鸣器标识显示使能
    boolean_t bM9En;        ///< 环境测温模式使能
    boolean_t bM10En;       ///< 小数点显示使能
    boolean_t bM11En;       ///< 电量显示使能
    boolean_t bM3En;        ///< 测温状态显示使能
    
    uint16_t  u16Num;       ///< 去掉小数点后的十进制温度值————温度示数显示"M1|1|2|3"

    en_tmp_mode_ref_t enTmpMode;    ///< 温度显示模式"摄氏度 or 华氏度"
    
}stc_lcd_display_cfg_t;


/*******************************************************************************
 ** \brief lcd相关函数声明
 ******************************************************************************/
extern void AppLcdInit(void);
extern void AppLcdStkLcdShow(uint32_t u32RamData);
extern void AppLcdDisplayUpdate(stc_lcd_display_cfg_t *pstcLcdDisplayCfg);
extern void AppLcdShowAll(void);
extern void AppLcdClearAll(void);
extern void AppLcdBlink(void); 
//@} // LCDGroup

#ifdef __cplusplus
#endif

#endif /* __APP_LCD_H__ */
/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/


