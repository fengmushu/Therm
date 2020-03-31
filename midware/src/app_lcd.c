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
/** \file lcd.c
 **
 ** lcd driver API.
 **
 **   - 2019-04-02    First Version
 **
 ******************************************************************************/

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "app_lcd.h"
#include "lcd.h"

/**
 ******************************************************************************
 ** \addtogroup AdcGroup
 ******************************************************************************/
//@{

/******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
///< LCD CODE for HC32L13x STK
volatile uint16_t u16StkLcdNum[10] = { 0x0F05,    //0
                                       0x0600,    //1
                                       0x0B06,    //2
                                       0x0F02,    //3
                                       0x0603,    //4
                                       0x0D03,    //5
                                       0x0D07,    //6
                                       0x0700,    //7
                                       0x0F07,    //8
                                       0x0F03     //9
                                     };

///< LCD CODE                                  
const volatile uint16_t u16LcdNumCode[15] = {
                                             0x0F05, //'0'
                                             0x0600, //'1'
                                             0x0D03, //'2'
                                             0x0F02, //'3'
                                             0x0606, //'4'
                                             0x0B06, //'5'
                                             0x0B07, //'6'
                                             0x0E00, //'7'
                                             0x0F07, //'8'
                                             0x0F06, //'9'
                                             0x0100, //'_'
                                             0x0105, //'L'
                                             0x0303, //'o'
                                             0x0607, //'H'
                                             0x0001, //'i'
                                            };    

#define LCD_M6_Msk  0x0008u;    //< u16RAM0L
#define LCD_M5_Msk  0x0008u;    //< u16RAM0H
#define LCD_M2_Msk  0x0008u;    //< u16RAM1L
#define LCD_M1_Msk  0x0008u;    //< u16RAM1H
#define LCD_M7_Msk  0x0800u;    //< u16RAM2L
#define LCD_M8_Msk  0x0400u;    //< u16RAM2L
#define LCD_M9_Msk  0x0200u;    //< u16RAM2L
#define LCD_M10_Msk 0x0100u;    //< u16RAM2L
#define LCD_M11_Msk 0x0001u;    //< u16RAM2L
#define LCD_M3_Msk  0x0004u;    //< u16RAM2L

/*****************************************************************************
 * Function implementation - global ('extern') and local ('static')
 *****************************************************************************/

///< LCD 初始化
void AppLcdInit(void)
{
    stc_lcd_cfg_t LcdInitStruct;
    stc_lcd_segcom_t LcdSegCom;

    Sysctrl_SetPeripheralGate(SysctrlPeripheralLcd,TRUE);  //LCD外设时钟打开
    
    // Sysctrl_SetRCLTrim(SysctrlRclFreq32768);                ///< 配置内部低速时钟频率为32.768kHz
    // Sysctrl_ClkSourceEnable(SysctrlClkRCL,TRUE);            ///< 使能RCL时钟
    
    LcdSegCom.u32Seg0_31 = 0xfffffc00;                              ///< 配置LCD_POEN0寄存器 开启SEG0~SEG9
    LcdSegCom.stc_seg32_51_com0_8_t.seg32_51_com0_8     = 0xffffffff;   ///< 初始化LCD_POEN1寄存器 全部关闭输出端口
    LcdSegCom.stc_seg32_51_com0_8_t.segcom_bit.Com0_3   = 0;          ///< 使能COM0~COM3
    LcdSegCom.stc_seg32_51_com0_8_t.segcom_bit.Mux      = 0;             ///< Mux=0,Seg32_35=0,BSEL=1表示:选择外部电容工作模式，内部电阻断路
    LcdSegCom.stc_seg32_51_com0_8_t.segcom_bit.Seg32_35 = 0;
    Lcd_SetSegCom(&LcdSegCom);                                      ///< LCD COMSEG端口配置

    LcdInitStruct.LcdBiasSrc = LcdExtCap;                               ///< 电容分压模式，需要外部电路配合
    LcdInitStruct.LcdDuty    = LcdDuty4;                                ///< 1/4 duty
    LcdInitStruct.LcdBias    = LcdBias3;                                ///< 1/3 BIAS
    LcdInitStruct.LcdCpClk   = LcdClk2k;                                ///< 电压泵时钟频率选择2kHz
    LcdInitStruct.LcdScanClk = LcdClk128hz;                             ///< LCD扫描频率选择128Hz
    LcdInitStruct.LcdMode    = LcdMode0;                                ///< 选择模式0
    LcdInitStruct.LcdClkSrc  = LcdRCL;                                  ///< LCD时钟选择RCL
    LcdInitStruct.LcdEn      = LcdEnable;                               ///< 使能LCD模块
    Lcd_Init(&LcdInitStruct);
}

/**
 ******************************************************************************
 ** \brief  LCD RAM 数据更新函数
 **
 ** \param  u32RamData   十进制数据(最大4位)
 **
 ** \retval NULL
 **
 ******************************************************************************/
void AppLcdStkLcdShow(uint32_t u32RamData)
{
    volatile uint32_t u32LcdNum;
    
    ///< 求均值
    u32LcdNum = u32RamData;
    
    M0P_LCD->RAM0 = ((u16StkLcdNum[(u32LcdNum/100)%10])<<16)|0;//(u16StkLcdNum[(u32LcdNum/1000)%10]);
    M0P_LCD->RAM1 = (u16StkLcdNum[(u32LcdNum%10)]<<16)|(u16StkLcdNum[(u32LcdNum/10)%10]);

    M0P_LCD->RAM1 |= 0x00080000;
}

/**
 ******************************************************************************
 ** \brief  LCD RAM 全显
 **
 ** \param  u32RamData   十进制数据(最大4位)
 **
 ** \retval NULL
 **
 ******************************************************************************/
void AppLcdShowAll(void)
{    
    M0P_LCD->RAM0 = 0xFFFFFFFF;
    M0P_LCD->RAM1 = 0xFFFFFFFF;
    M0P_LCD->RAM2 = 0xFFFFFFFF;
}

/**
 ******************************************************************************
 ** \brief  LCD RAM 无显
 **
 ** \param  u32RamData   十进制数据(最大4位)
 **
 ** \retval NULL
 **
 ******************************************************************************/
void AppLcdClearAll(void)
{    
    M0P_LCD->RAM0 = 0u;
    M0P_LCD->RAM1 = 0u;
    M0P_LCD->RAM2 = 0u;
}

/**
 ******************************************************************************
 ** \brief  LCD RAM 数据更新函数
 **
 ** \param  pstcLcdDisplayCfg @ref stc_lcd_display_cfg_t
 **
 ** \retval enRet 成功或失败
 **
 ******************************************************************************/
void AppLcdDisplayUpdate(stc_lcd_display_cfg_t *pstcLcdDisplayCfg)
{
    volatile uint16_t *pu16LcdRam;

    M0P_LCD->RAM0 = 0;
    M0P_LCD->RAM1 = 0;
    M0P_LCD->RAM2 = 0;
    
    pu16LcdRam = (volatile uint16_t *)(&M0P_LCD->RAM0);
    
    pu16LcdRam[4] = pstcLcdDisplayCfg->enTmpMode;
//    pu16LcdRam[4] = u16LcdNumCode[(pstcLcdDisplayCfg->u16Num%10)];
//    pstcLcdDisplayCfg->u16Num /=10;    
 

    
    if(0xFFFFu == pstcLcdDisplayCfg->u16Num)
    {
        pu16LcdRam[3] = u16LcdNumCode[10];
        pu16LcdRam[2] = u16LcdNumCode[10];
        pu16LcdRam[1] = u16LcdNumCode[10];
    }
    else if(0xFFFEu == pstcLcdDisplayCfg->u16Num)
    {
        pu16LcdRam[3] = 0u;
        pu16LcdRam[2] = 0u;
        pu16LcdRam[1] = 0u;
        
    }
    else
    {
        pu16LcdRam[3] = u16LcdNumCode[(pstcLcdDisplayCfg->u16Num%10)];
        pu16LcdRam[2] = u16LcdNumCode[((pstcLcdDisplayCfg->u16Num/10)%10)];
        pu16LcdRam[1] = u16LcdNumCode[((pstcLcdDisplayCfg->u16Num/100)%10)];
        
        if(1 == pstcLcdDisplayCfg->u16Num/1000) {pu16LcdRam[1] |= LCD_M1_Msk; }
    }
    if(TRUE == pstcLcdDisplayCfg->bM6En) {pu16LcdRam[4] |= LCD_M6_Msk; }
    if(TRUE == pstcLcdDisplayCfg->bM5En) {pu16LcdRam[3] |= LCD_M5_Msk; }
    if(TRUE == pstcLcdDisplayCfg->bM2En) {pu16LcdRam[2] |= LCD_M2_Msk; }
    if(TRUE == pstcLcdDisplayCfg->bM7En) {pu16LcdRam[0] |= LCD_M7_Msk; }
    if(TRUE == pstcLcdDisplayCfg->bM8En) {pu16LcdRam[0] |= LCD_M8_Msk; }
    if(TRUE == pstcLcdDisplayCfg->bM9En) {pu16LcdRam[0] |= LCD_M9_Msk; }
    if(TRUE == pstcLcdDisplayCfg->bM10En){pu16LcdRam[0] |= LCD_M10_Msk;}
    if(TRUE == pstcLcdDisplayCfg->bM11En){pu16LcdRam[0] |= LCD_M11_Msk;}
    if(TRUE == pstcLcdDisplayCfg->bM3En) {pu16LcdRam[0] |= LCD_M3_Msk; }

}

void AppLcdBlink(void)
{
    AppLcdShowAll();
    delay1ms(400);
    AppLcdClearAll();
    delay1ms(400);
    AppLcdShowAll();
    delay1ms(400);
    AppLcdClearAll();
    delay1ms(400);
    AppLcdShowAll();
}

/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

