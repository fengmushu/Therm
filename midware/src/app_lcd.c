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
 **   - 2020-04-03    First Version
 **
 ******************************************************************************/

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "app_lcd.h"
#include "lcd.h"
#include "app.h"

/* lcd symbol bit */
#define LCD_SYM_MSK 0x08


typedef enum enLcdSymbolType
{
    BATTERY_SYM    = 0,     /* 电池 */
    LOG_T_DT_SYM   = 1,     /* 记录温度的点号 */
    FAH_SYM        = 2,     /* 华氏度 */
    CELS_SYM       = 3,     /* 摄氏度 */
    LOCK_SYM       = 4,     /* 锁标记 */
    LOG_SYM        = 5,     /* log标记 */
    BUZZER_SYM     = 6,     /* 蜂鸣器 */
    TEMP_DOT_SYM   = 7,     /* 当前温度的点号 */
    BODY_SYM       = 8,     /* 人体测温标记 */
    SURFACE_SYM    = 9,     /* 物体测温标记 */
    MAX_SYM
}enLcdSymbolType_t;

#if 0 /* if no memory, */
typedef enum enLcdChar
{
    CHAR_F = 10,    /* 'F' */
    CHAR_L = 11,    /* 'L' */
    CHAR_O = 12,    /* 'o' */
    CHAR_N = 13,    /* 'N' */
    CHAR_U = 14,    /* 'u' */
    CHAR_P = 15,    /* 'P' */
}enLcdChar_t;
#endif  


/******************************************************************************
 ** LCD Data Map Config
 *****************************************************************************/
typedef struct stc_lcd_display_cfg
{
    boolean_t       Sym_display[MAX_SYM]; /* lcd符号显示位 */
    enTempMode_t    enTempMode;    /* 温度显示模式"摄氏度 or 华氏度" */
    enCheckMode_t   enCheckMode;   /* 检测模式显示 */
    enStrType_t     enStrType;     /* 显示字符串的类型 */

    boolean_t  bRawNum;         /* 是否显示原始格式 */
    boolean_t  bTempDis;        /* 是否显示当前温度 */
    boolean_t  bLogTempDis;     /* 是否显示log温度 */
    boolean_t  bRaw_dis_dot;    /* 是否显示点号 */

    boolean_t  bLogChanged;     /* log温度是否修改 */
    boolean_t  bTempChanged;    /* 温度是否修改 */
    
    uint8_t   bRaw_digits_num;  /* 数字个数 */
    uint16_t  u16LogIndex;   /* log index */
    int16_t   u16Temp;       /* *10去掉小数点后的十进制温度值" */
    uint16_t  u16LogTemp;    /* log index对应的温度，*10去掉小数点后的值 */
    
}stc_lcd_display_cfg_t;


/*****************************************************************************
 * Function implementation - global ('extern') and local ('static')
 *****************************************************************************/
static stc_lcd_display_cfg_t gstcLcdDisplayCfg = {0};

/* our lcd num */
static const uint16_t s_LcdNumCode[11] = {
    0x0f05, //0
    0x0005, //1
    0x0D03, //2
    0x0907, //3
    0x0207, //4
    0x0b06, //5
    0x0f06, //6
    0x0105, //7
    0x0f07, //8
    0x0b07, //9
    0x0002, //-
};

/* 0x0702 F 0x0e00 L 0x0c06 o 0x0705 N 0x0607 H*/
/* lcd string code */
static const uint16_t s_LcdStrCode[Str_MAX -1][2] = 
{
    {0x0702, 0x0005},            /* F1 */
    {0x0702, 0x0D03},            /* F2 */
    {0x0702, 0x0907},            /* F3 */
    {0x0702, 0x0207},            /* F4 */
    {0x0c06, 0x0705},            /* oN */
    {0x0c06, 0x0702},            /* oFF */
    {0x0e00, 0x0c06},            /* Lo */
    {0x0607, 0x0005},            /* HI */
};
    

static inline void sAppLcdDisplaySymbol(volatile uint16_t* LcdRam, enLcdSymbolType_t Type, boolean_t display)
{
    /* 现在的设计ram index刚好对应type值 */
    if (display)
    {
        LcdRam[Type] |= LCD_SYM_MSK;
    }
    else
    {
        LcdRam[Type] &= ~LCD_SYM_MSK;
    }
}


static inline void sAppLcdDisplayNumber(uint16_t Display, volatile uint16_t* LcdRam, uint16_t RamIndex, 
        uint16_t Max_number, int8_t MinDigit)
{
    if (Display > Max_number)
    {
        Display = Max_number;
    }
    
    while (Display > 0 || MinDigit > 0)
    {
        LcdRam[RamIndex++] = s_LcdNumCode[Display%10];
        Display /= 10;
        MinDigit--;
    }
    
    return;
}

static inline void sAppLcdDisplayRawNumber(int16_t Display, volatile uint16_t* LcdRam, uint16_t RamIndex, 
        int8_t MinDigit)
{
    boolean_t is_negative = Display < 0 ? TRUE : FALSE;
    if ( Display < -999 || Display > 9999)
    {
        DBG_PRINT("%s: error display=%d\r\n", __func__, Display);
        Display = 0;
    }
    
    if (is_negative)Display = - Display;

    while (Display != 0 || MinDigit > 0)
    {
        LcdRam[RamIndex++] = s_LcdNumCode[Display%10];
        Display /= 10;
        MinDigit--;
    }
    
    if (is_negative)
    {
        LcdRam[RamIndex] = s_LcdNumCode[10];
    }
    
    return;
}

static inline void sAppLcdNoDisplayNumber(volatile uint16_t* LcdRam, uint16_t RamIndex, uint8_t digit_num)
{  
    while (digit_num > 0)
    {
        LcdRam[RamIndex++] = 0;
        digit_num--;
    }
    
    return;
}

/* temp display: digit[1-4] ->pu16LcdRam[9-6] */
static inline void sAppLcdDisplayTemp(stc_lcd_display_cfg_t *pstcLcdDisplayCfg)
{
    volatile uint16_t *pu16LcdRam = (volatile uint16_t *)(&M0P_LCD->RAM0);

    /* display number */
    sAppLcdDisplayNumber(pstcLcdDisplayCfg->u16Temp, pu16LcdRam, 6, 9999, 2);
    sAppLcdDisplaySymbol(pu16LcdRam, TEMP_DOT_SYM, TRUE);
}

/* String disply: digit[2-4] -> pu16LcdRam[8-6] */
static inline void sAppLcdDisplayStr(stc_lcd_display_cfg_t *pstcLcdDisplayCfg)
{
    volatile uint16_t *pu16LcdRam = (volatile uint16_t *)(&M0P_LCD->RAM0);
    uint16_t RamIndex = 8;
    uint8_t i;

    pu16LcdRam[RamIndex--] = s_LcdStrCode[pstcLcdDisplayCfg->enStrType][0];
    pu16LcdRam[RamIndex--] = s_LcdStrCode[pstcLcdDisplayCfg->enStrType][1];
    
    /* for Str_OFF, has 3 chars */
    if (pstcLcdDisplayCfg->enStrType == Str_OFF)
    {
        pu16LcdRam[RamIndex] = s_LcdStrCode[pstcLcdDisplayCfg->enStrType][1];
    }
    return;
}

/* log temp index display: digit[5-6] ->pu16LcdRam[5-4]*/
static inline void sAppLcdDisplayLogIndex(stc_lcd_display_cfg_t *pstcLcdDisplayCfg)
{
    volatile uint16_t *pu16LcdRam = (volatile uint16_t *)(&M0P_LCD->RAM0);

    /* display number */
    sAppLcdDisplayNumber(pstcLcdDisplayCfg->u16LogIndex, pu16LcdRam, 4, 99, 2);
}

/* log temp display: digit[7-10] ->pu16LcdRam[3-0] */
static inline void sAppLcdDisplayLogTemp(stc_lcd_display_cfg_t *pstcLcdDisplayCfg)
{
    volatile uint16_t *pu16LcdRam = (volatile uint16_t *)(&M0P_LCD->RAM0);

    /* display number */
    sAppLcdDisplayNumber(pstcLcdDisplayCfg->u16LogTemp, pu16LcdRam, 0, 9999, 2);
    sAppLcdDisplaySymbol(pu16LcdRam, LOG_T_DT_SYM, TRUE);
}

/* log temp index display: digit[5-6] ->pu16LcdRam[5-4]*/
static inline void sAppLcdNoDisplayLog(stc_lcd_display_cfg_t *pstcLcdDisplayCfg)
{
    volatile uint16_t *pu16LcdRam = (volatile uint16_t *)(&M0P_LCD->RAM0);

    /* no display number */
    sAppLcdNoDisplayNumber(pu16LcdRam, 4, 2);
    sAppLcdNoDisplayNumber(pu16LcdRam, 0, 4);
    sAppLcdDisplaySymbol(pu16LcdRam, LOG_T_DT_SYM, FALSE);
    sAppLcdDisplaySymbol(pu16LcdRam, LOG_SYM, FALSE);
}


static inline void sAppLcdSetSymbol(stc_lcd_display_cfg_t *pstcLcdDisplayCfg, enLcdSymbolType_t type, boolean_t v)
{
    pstcLcdDisplayCfg->Sym_display[type] = v;
    return;
}


///< LCD 初始化
void AppLcdInit(void)
{
    stc_lcd_cfg_t LcdInitStruct;
    stc_lcd_segcom_t LcdSegCom;

    Sysctrl_SetPeripheralGate(SysctrlPeripheralLcd,TRUE);  //LCD外设时钟打开
    
    Sysctrl_SetRCLTrim(SysctrlRclFreq32768);                ///< 配置内部低速时钟频率为32.768kHz
    Sysctrl_ClkSourceEnable(SysctrlClkRCL,TRUE);            ///< 使能RCL时钟
    
    LcdSegCom.u32Seg0_31 = 0xFFE00000;                              ///< 配置LCD_POEN0寄存器 开启SEG0~SEG9, SEG10-19
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

    AppLcdDisplayClear();
    AppLcdClearAll();
}

///< LCD 初始化模式显示
void AppLcdDisplayClear(void)
{
    M0P_LCD->RAM0 = 0x0;
    M0P_LCD->RAM1 = 0x0;
    M0P_LCD->RAM2 = 0x0;
    M0P_LCD->RAM3 = 0x0;
    M0P_LCD->RAM4 = 0x0;
    return;
}

void AppLcdDisplayAll(void)
{
    M0P_LCD->RAM0 = 0xffffffff;
    M0P_LCD->RAM1 = 0xffffffff;
    M0P_LCD->RAM2 = 0xffffffff;
    M0P_LCD->RAM3 = 0xffffffff;
    M0P_LCD->RAM4 = 0xffffffff;      
    return;
}

void AppLcdSetLock(boolean_t display)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    sAppLcdSetSymbol(pstcLcdDisplayCfg, LOCK_SYM, display);
    return;
}

void AppLcdSetBuzzer(boolean_t display)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    sAppLcdSetSymbol(pstcLcdDisplayCfg, BUZZER_SYM, display);
    return;
}

void AppLcdSetBattery(boolean_t display)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    sAppLcdSetSymbol(pstcLcdDisplayCfg, BATTERY_SYM, display);
    return;
}


void AppLcdSetCheckMode(enCheckMode_t CheckMode, boolean_t display)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    pstcLcdDisplayCfg->enCheckMode = CheckMode;
    switch(CheckMode)
    {
        case Body:
            sAppLcdSetSymbol(pstcLcdDisplayCfg, BODY_SYM, display);
            sAppLcdSetSymbol(pstcLcdDisplayCfg, SURFACE_SYM, FALSE);
            break;
                
        case Surface:
            sAppLcdSetSymbol(pstcLcdDisplayCfg, BODY_SYM, FALSE);
            sAppLcdSetSymbol(pstcLcdDisplayCfg, SURFACE_SYM, display);
            break;
            
        case CheckNone:
        default:
            sAppLcdSetSymbol(pstcLcdDisplayCfg, BODY_SYM, FALSE);
            sAppLcdSetSymbol(pstcLcdDisplayCfg, SURFACE_SYM, FALSE);
            break;
                
            
    }
}

void AppLcdSetTempMode(enTempMode_t TempMode, boolean_t display)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    pstcLcdDisplayCfg->enTempMode = TempMode;
    switch(TempMode)
    {
        case Celsius:
            sAppLcdSetSymbol(pstcLcdDisplayCfg, CELS_SYM, display);
            sAppLcdSetSymbol(pstcLcdDisplayCfg, FAH_SYM, FALSE);
            break;
                
        case Fahrenheit:
            sAppLcdSetSymbol(pstcLcdDisplayCfg, CELS_SYM, FALSE);
            sAppLcdSetSymbol(pstcLcdDisplayCfg, FAH_SYM, display);
            break;
            
        case TempNone:
        default:
            sAppLcdSetSymbol(pstcLcdDisplayCfg, CELS_SYM, FALSE);
            sAppLcdSetSymbol(pstcLcdDisplayCfg, FAH_SYM, FALSE);
            break;
                
            
    }
    return;
}


void AppLcdSetTemp(uint16_t Temp)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    pstcLcdDisplayCfg->u16Temp = (int16_t)Temp;
    pstcLcdDisplayCfg->bTempDis = TRUE;
    /* 共用数码管，字符显示应关闭 */
    pstcLcdDisplayCfg->enStrType = Str_NONE; 
    pstcLcdDisplayCfg->bRawNum = FALSE;
    sAppLcdSetSymbol(pstcLcdDisplayCfg, TEMP_DOT_SYM, TRUE);
    pstcLcdDisplayCfg->bTempChanged = TRUE;
    return;
}

void AppLcdClearTemp(void)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    pstcLcdDisplayCfg->bTempDis = FALSE;
    /* 共用数码管，字符显示应关闭 */
    pstcLcdDisplayCfg->enStrType = Str_NONE;
    pstcLcdDisplayCfg->bRawNum = FALSE;
    sAppLcdSetSymbol(pstcLcdDisplayCfg, TEMP_DOT_SYM, FALSE);
    pstcLcdDisplayCfg->bTempChanged = TRUE;
    return;
}

void AppLcdSetLogTemp(uint16_t Temp, uint16_t Index)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    pstcLcdDisplayCfg->u16LogTemp = Temp;
    pstcLcdDisplayCfg->u16LogIndex = Index;
    pstcLcdDisplayCfg->bLogTempDis = TRUE;
    sAppLcdSetSymbol(pstcLcdDisplayCfg, LOG_SYM, TRUE);
    sAppLcdSetSymbol(pstcLcdDisplayCfg, LOG_T_DT_SYM, TRUE);
    pstcLcdDisplayCfg->bLogChanged = TRUE;
    return;
}

void AppLcdClearLogTemp(void)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    pstcLcdDisplayCfg->bLogTempDis = FALSE;
    sAppLcdSetSymbol(pstcLcdDisplayCfg, LOG_SYM, FALSE);
    sAppLcdSetSymbol(pstcLcdDisplayCfg, LOG_T_DT_SYM, FALSE);
    pstcLcdDisplayCfg->bLogChanged = TRUE;
    return;
}


void AppLcdSetString(enStrType_t StrType)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    if (StrType >= Str_MAX)
    {
        printf("%s: Invalid StrType =%u\n", __func__, StrType);
        return;
    }
    pstcLcdDisplayCfg->enStrType = StrType;
    /* 共用数码管，温度显示应关闭 */
    pstcLcdDisplayCfg->bTempDis = FALSE;
    pstcLcdDisplayCfg->bRawNum = FALSE;
    sAppLcdSetSymbol(pstcLcdDisplayCfg, TEMP_DOT_SYM, FALSE);
    pstcLcdDisplayCfg->bTempChanged = TRUE;
    return;
}

void AppLcdSetRawNumber(int16_t Temp, boolean_t dis_dot, uint8_t min_digits)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    pstcLcdDisplayCfg->u16Temp = Temp;
    pstcLcdDisplayCfg->bRawNum = TRUE;
    pstcLcdDisplayCfg->bRaw_digits_num = min_digits > 4 ? 4 : min_digits;
    
    pstcLcdDisplayCfg->bTempDis = FALSE;
    /* 共用数码管，字符显示应关闭 */
    pstcLcdDisplayCfg->enStrType = Str_NONE; 
    sAppLcdSetSymbol(pstcLcdDisplayCfg, TEMP_DOT_SYM, dis_dot);
    pstcLcdDisplayCfg->bTempChanged = TRUE;
    return;
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
void AppLcdDisplayUpdate()
{
    volatile uint16_t *pu16LcdRam;
    uint8_t i = 0;
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;

    //AppLcdDisplayClear();
    pu16LcdRam = (volatile uint16_t *)(&M0P_LCD->RAM0);
       
    /* temp display */
    if (pstcLcdDisplayCfg->bTempChanged)
    {
        sAppLcdNoDisplayNumber(pu16LcdRam, 6, 4);
        if (pstcLcdDisplayCfg->bTempDis)
        {
            sAppLcdDisplayTemp(pstcLcdDisplayCfg);
        }
        else if (pstcLcdDisplayCfg->enStrType != Str_NONE)
        {
            sAppLcdDisplayStr(pstcLcdDisplayCfg);
        }
        else if (pstcLcdDisplayCfg->bRawNum)
        {
            sAppLcdDisplayRawNumber(pstcLcdDisplayCfg->u16Temp, 
                pu16LcdRam, 6, pstcLcdDisplayCfg->bRaw_digits_num);
        }
        pstcLcdDisplayCfg->bTempChanged = FALSE;
    }

    if (pstcLcdDisplayCfg->bLogChanged)
    {
        /* log index display */
        sAppLcdNoDisplayLog(pstcLcdDisplayCfg);
        if (pstcLcdDisplayCfg->bLogTempDis)
        {
            sAppLcdDisplayLogIndex(pstcLcdDisplayCfg);
            
            /* log temp display */        
            sAppLcdDisplayLogTemp(pstcLcdDisplayCfg);
        }
        pstcLcdDisplayCfg->bLogChanged = FALSE;
    }

    /* symbols display */
    for (i = 0; i < MAX_SYM; ++i)
    {
        sAppLcdDisplaySymbol(pu16LcdRam, i, pstcLcdDisplayCfg->Sym_display[i]);
    }

    delay1ms(20); /* delay for display ok */
    
    return;
}

void AppLcdClearAll(void)
{
    memset(&gstcLcdDisplayCfg, 0, sizeof(gstcLcdDisplayCfg));
    AppLcdClearTemp();
    AppLcdClearLogTemp();
    return;
}


void AppLcdBlink(void)
{
    uint8_t cnt = 5;
    while (cnt-- > 0)
    {
        if (cnt % 2 == 0)
        {
            AppLcdDisplayAll();
        }
        else
        {
            AppLcdDisplayClear();
        }
        delay1ms(400);
    }
}

#define LCD_DEBUG 1
#if LCD_DEBUG
void AppLcdDebug(void)
{
    int8_t test_cnt = 1;
    while ((test_cnt--) > 0)
    {
        uint16_t i = 0;  
        uint16_t tmp = 0;
        AppLcdBlink();          ///< 初次上电开机LCD全屏显示闪烁两次
        AppLcdDisplayClear();    ///< LCD 初始状态显示

        AppLcdSetRawNumber(-10, TRUE, 2);
        AppLcdDisplayUpdate();
        delay1ms(500);

        AppLcdSetRawNumber(-10, FALSE, 2);
        AppLcdDisplayUpdate();
        delay1ms(500);
        
        AppLcdSetRawNumber(-10, TRUE, 3);
        AppLcdDisplayUpdate();
        delay1ms(500);

        AppLcdSetRawNumber(-10, TRUE, 4);
        AppLcdDisplayUpdate();
        delay1ms(500);

        AppLcdSetRawNumber(-100, TRUE, 4);
        AppLcdDisplayUpdate();
        delay1ms(500);

        AppLcdSetBattery(TRUE);
        AppLcdDisplayUpdate();
        delay1ms(500);

        AppLcdSetBuzzer(TRUE);
        AppLcdDisplayUpdate();
        delay1ms(500);

        AppLcdSetCheckMode(Surface, TRUE);
        AppLcdDisplayUpdate();
        delay1ms(500);

        AppLcdSetCheckMode(Body, TRUE);
        AppLcdDisplayUpdate();
        delay1ms(500);

        AppLcdSetLock(TRUE);
        AppLcdDisplayUpdate();
        delay1ms(500);

        AppLcdSetTempMode(Celsius, TRUE);
        AppLcdDisplayUpdate();
        delay1ms(500);

        AppLcdSetTempMode(Fahrenheit, TRUE);
        AppLcdDisplayUpdate();
        delay1ms(500);
               
        for (i = 0; i < Str_MAX; ++i)
        {
            AppLcdSetString(i);
            AppLcdDisplayUpdate();
            delay1ms(500);
        }

        AppLcdClearAll();
        AppLcdDisplayUpdate();
        delay1ms(1000);
        
        for (i = 0; i < 40; ++i)
        {
            tmp = 0;
            tmp += (i >= 30 ? (i - 30) : 0)*1000;
            tmp += ((i >= 20 && i < 30) ? (i -20) : 0)*100;
            tmp += ((i >= 10 && i < 20) ? (i -10) : 0)*10;
            tmp += (i < 10) ? i:0;

            AppLcdSetTemp(tmp);
            AppLcdSetLogTemp(tmp, tmp > 100 ? tmp/100 : tmp);

            AppLcdSetTempMode(Celsius, i%2);

					
            AppLcdSetBattery(i%2);

            AppLcdDisplayUpdate();

            delay1ms(500);
        }
    }
}

#endif

/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

