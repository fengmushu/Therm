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
/** \file app.h
 **
 ** BGR 数据结构及API声明.
 **
 **   - 2018-04-21  LuX V1.0.
 **
 ******************************************************************************/

#ifndef __APP_H__
#define __APP_H__

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "ddl.h"
#include "lvd.h"
#include "pca.h"
#include "rtc.h"
#include "gpio.h"
#include "uart.h"
#include "app_adc.h"
#include "nna.h"
#include "flash.h"

#define DBG_PRINT printf

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/**
 ******************************************************************************
 ** \defgroup BGR
  **
 ******************************************************************************/
//@{

/******************************************************************************
 * Global type definitions
 ******************************************************************************/
typedef enum en_led_colour
{
    LedRed       = 0x00u,                 ///< 红色
    LedLightBlue = 0x40u,                 ///< 浅蓝色
}en_led_colour_t;

/******************************************************************************
 * Global definitions
 ******************************************************************************/
#define VIRL_PARA_ADDR           (stTherBoardPara.u32BlackBodyTempLAddr)                                   ///< 红外低温标定数据存放地址
#define VIRL_PARA_DATA           (*((volatile uint32_t*)VIRL_PARA_ADDR))    ///< 红外低温标定数据
#define VIRH_PARA_ADDR           (stTherBoardPara.u32BlackBodyTempHAddr)                                   ///< 红外高温标定数据存放地址
#define VIRH_PARA_DATA           (*((volatile uint32_t*)VIRH_PARA_ADDR))    ///< 红外高温标定数据
    

#define USERKEYTRUE     (0xFFFFFFFFu)
#define USERKEYFALSE    (0x00000000u)

#define CHARGEEMPTY     (0x00000000u)
#define CHARGEFULL      (0xFFFFFFFFu)

/******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/******************************************************************************
 * Global function prototypes (definition in C source)                        
 ******************************************************************************/
///< VCC电压监测功能初始化
extern void AppVolMonitorInit(void);
///< 蜂鸣器滴滴
extern void AppBeepBlink(uint32_t u32FreqIndex);

///< 自动关机模块初始化
extern void AppPowerOffModuleInit(void);
extern en_result_t AppRtcFeed(void);

//@} // APP Group
///< LED背光灯控制
extern void AppLedEnable(en_led_colour_t enLedColour);
extern void AppLedDisable(void);   

///< UART 初始化
extern void AppUartInit(void);

///< 参数标定区初始化
extern void AppParaAreaInit(void);
///< VIR黑体 校准系数标定
extern void AppVirLParaMark(uint32_t u32VirLDataCal);
extern void AppVirHParaMark(uint32_t u32VirHDataCal);
extern uint16_t AppVirLParaGet(void);
extern uint16_t AppVirHParaGet(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_H__ */
/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

