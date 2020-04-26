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
#include "app_data.h"
#include "lcd.h"
#include "app.h"

/* lcd symbol bit */
#define LCD_SYM_MSK 0x08

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
    boolean_t  bLogRawNum;       /* 日志数字显示原始格式 */

    boolean_t  bLogChanged;     /* log温度是否修改 */
    boolean_t  bTempChanged;    /* 温度是否修改 */
    
    uint8_t   bRaw_digits_num;  /* 数字个数 */
    uint8_t   bLogRaw_digits_num;  /* 数字个数 */
    
    int16_t   LogIndex;   /* log index */
    int16_t   u16Temp;       /* *10去掉小数点后的十进制温度值" */
    int16_t   u16LogTemp;    /* log index对应的温度，*10去掉小数点后的值 */
    
}stc_lcd_display_cfg_t;


/*****************************************************************************
 * Function implementation - global ('extern') and local ('static')
 *****************************************************************************/
static stc_lcd_cfg_t LcdConfig;
static stc_lcd_display_cfg_t gstcLcdDisplayCfg = {0};

static const uint8_t s_NumPos[4] = {
    2,   //2-1seg
    4, 	 //4-3seg
    10,  //10-9seg
    12,  //12-11seg
};

static const uint8_t s_LogNumPos[2] = {
    6, //8-7seg
    8, //6-5seg
};

static const uint8_t s_SymbolPos[14] = {
    13,     /* 电池 */
    9,      /* 电池余量1 */
    0,      /* 电池余量2 */
    0,      /* 电池余量3 */
    13,     /* 华氏度 */
    13,     /* 摄氏度 */
    7,      /* log标记 */
    13,     /* 蜂鸣器 */
    11,     /* 当前温度的点号 */
    0,      /* 人体测温标记 */
    5,      /* 物体测温标记 */
    3,      /* 难过表情 */
    1,      /* 微笑表情 */
    0,      /* 蓝牙 */
};

static const uint8_t s_SymbolCode[14] = {
    0x1,     /* 电池 */
    0x1,     /* 电池余量1 */
    0x4,    /* 电池余量2 */
    0x8,    /* 电池余量3 */
    0x4,     /* 华氏度 */
    0x8,     /* 摄氏度 */
    0x1,     /* log标记 */
    0x2,     /* 蜂鸣器 */
    0x1,     /* 当前温度的点号 */
    0x2,    /* 人体测温标记 */
    0x1,     /* 物体测温标记 */
    0x1,    /* 难过表情 */
    0x1,    /* 微笑表情 */
    0x1,    /* 蓝牙 */
};


/* our lcd num , hight 4bits low 4bits */
static const uint8_t s_LcdNumCode[12][2] = {
    {0xf, 0xa}, //0
    {0x6, 0x0}, //1
    {0xd, 0x6}, //2
    {0xf, 0x4}, //3
    {0x6, 0xc}, //4
    {0xb, 0xc}, //5
    {0xb, 0xe}, //6
    {0xe, 0x0}, //7
    {0xf, 0xe}, //8
    {0xf, 0xc}, //9
    {0x0, 0x4}, //-
    {0x3, 0x6}, //'o'
};


/* lcd string code */
static const uint8_t s_LcdStrCode[Str_MAX -1][4] = 
{
    {0x8, 0xe, 0x6, 0x0},            /* F1 */
    {0x8, 0xe, 0xd, 0x6},            /* F2 */
    {0x8, 0xe, 0xf, 0x4},            /* F3 */
    {0x8, 0xe, 0x6, 0xc},            /* F4 */
    {0x3, 0x6, 0xe, 0xa},            /* oN */
    {0x8, 0xe, 0x8, 0xe},            /* oFF */
    {0x1, 0xa, 0x3, 0x6},            /* Lo */
    {0x6, 0xe, 0x6, 0x0},            /* HI */
    {0x0, 0x4, 0x0, 0x4}             /* -- */
};
    

static inline void sAppLcdDisplaySymbol(volatile uint8_t* LcdRam, enLcdSymbolType_t Type, boolean_t display)
{
    if (display)
    {
        LcdRam[s_SymbolPos[Type]] |= s_SymbolCode[Type];
    }
    else
    {
        LcdRam[s_SymbolPos[Type]] &= ~s_SymbolCode[Type];
    }
}


static inline void sAppLcdDisplayNumber(uint16_t Display, volatile uint8_t* LcdRam, 
        uint16_t Max_number, int8_t MinDigit)
{
    int8_t i = 3; 
    if (Display > Max_number)
    {
        Display = Max_number;
    }
    
    while (i >= 0 && (Display > 0 || MinDigit > 0))
    {
        LcdRam[s_NumPos[i]] = s_LcdNumCode[Display%10][0];
        LcdRam[s_NumPos[i] - 1] = s_LcdNumCode[Display%10][1];
        Display /= 10;
        MinDigit--;
        i--;
    }
    
    return;
}

static inline void sAppLcdDisplayRawNumber(int16_t Display, volatile uint8_t* LcdRam, 
        int8_t MinDigit)
{
    boolean_t is_negative = Display < 0 ? TRUE : FALSE;
    int8_t i = 3;
    
    if ( Display < -999 || Display > 9999)
    {
        DBG_PRINT("%s: error display=%d\r\n", __func__, Display);
        Display = 0;
    }
    
    if (is_negative)Display = - Display;
   
    while (i >= 0 && (Display > 0 || MinDigit > 0))
    {
        LcdRam[s_NumPos[i]] = s_LcdNumCode[Display%10][0];
        LcdRam[s_NumPos[i] - 1] = s_LcdNumCode[Display%10][1];
        Display /= 10;
        MinDigit--;
        i--;
    }
    
    if (is_negative)
    {
        if (i < 0) i = 0;
        LcdRam[s_NumPos[i]] = s_LcdNumCode[10][0];
        LcdRam[s_NumPos[i] - 1] = s_LcdNumCode[10][1];
    }
    
    return;
}

static inline void sAppLcdNoDisplayNumber(volatile uint8_t* LcdRam, uint8_t digit_num)
{  
    int8_t i = digit_num - 1;
    while (i >= 0)
    {
        LcdRam[s_NumPos[i]] = 0;
        LcdRam[s_NumPos[i] - 1] = 0;
        i--;
    }
    
    return;
}

/* temp display: digit[1-4] ->pu16LcdRam[9-6] */
static inline void sAppLcdDisplayTemp(stc_lcd_display_cfg_t *pstcLcdDisplayCfg)
{
    volatile uint8_t* LcdRam = (volatile uint8_t *)(&M0P_LCD->RAM0);

    /* display number */
    sAppLcdDisplayNumber(pstcLcdDisplayCfg->u16Temp, LcdRam, 9999, 2);
    sAppLcdDisplaySymbol(LcdRam, TEMP_DOT_SYM, TRUE);
}

/* String disply: digit[2-4] -> pu16LcdRam[8-6] */
static inline void sAppLcdDisplayStr(stc_lcd_display_cfg_t *pstcLcdDisplayCfg)
{
    volatile uint8_t *LcdRam = (volatile uint8_t *)(&M0P_LCD->RAM0);
    int8_t i = 2, j = 0;

    while (i < 4)
    {
        LcdRam[s_NumPos[i]] = s_LcdStrCode[pstcLcdDisplayCfg->enStrType][j++];
        LcdRam[s_NumPos[i]-1] = s_LcdStrCode[pstcLcdDisplayCfg->enStrType][j++];
        i++;
    }
     
    /* for Str_OFF, has 3 chars */
    if (pstcLcdDisplayCfg->enStrType == Str_OFF)
    {
        LcdRam[s_NumPos[1]]   = s_LcdNumCode[11][0];
        LcdRam[s_NumPos[1]-1] = s_LcdNumCode[11][1];
    }
    else if (pstcLcdDisplayCfg->enStrType == Str_LINE) /* Str_LINE has 4 chars */
    {
        i = 0;
        while (i < 2)
        {
            LcdRam[s_NumPos[i]]   = s_LcdNumCode[10][0];
            LcdRam[s_NumPos[i]-1] = s_LcdNumCode[10][1];
            i++;
        }
    }
    return;
}

/* log temp index display: digit[5-6] ->pu16LcdRam[5-4]*/
static inline void sAppLcdDisplayLogIndex(stc_lcd_display_cfg_t *pstcLcdDisplayCfg)
{
    volatile uint8_t *LcdRam = (volatile uint8_t *)(&M0P_LCD->RAM0);
    uint16_t Display = pstcLcdDisplayCfg->LogIndex;
    int8_t MinDigit = 2;

    int8_t i = 1; 
    if (Display > 99)
    {
        Display = 99;
    }
    
    while (i >= 0 && (Display > 0 || MinDigit > 0))
    {
        LcdRam[s_LogNumPos[i]] = s_LcdNumCode[Display%10][0];
        LcdRam[s_LogNumPos[i] - 1] = s_LcdNumCode[Display%10][1];
        Display /= 10;
        MinDigit--;
        i--;
    }
    
    return;
}

/* log temp index display: digit[5-6] ->pu16LcdRam[5-4]*/
static inline void sAppLcdNoDisplayLog(stc_lcd_display_cfg_t *pstcLcdDisplayCfg)
{
    volatile uint8_t *LcdRam = (volatile uint8_t *)(&M0P_LCD->RAM0);

    int8_t i = 1;
    while (i >= 0)
    {
        LcdRam[s_LogNumPos[i]] = 0;
        LcdRam[s_LogNumPos[i] - 1] = 0;
        i--;
    }
    
    return;
    
    /* no display log */
    sAppLcdDisplaySymbol(LcdRam, LOG_SYM, FALSE);
}


static inline void sAppLcdSetSymbol(stc_lcd_display_cfg_t *pstcLcdDisplayCfg, enLcdSymbolType_t type, boolean_t v)
{
    pstcLcdDisplayCfg->Sym_display[type] = v;
    return;
}

void AppLcdEnable(void)
{
    LcdConfig.LcdEn = LcdEnable;
    Lcd_Init(&LcdConfig);
}

void AppLcdDisable(void)
{
    LcdConfig.LcdEn = LcdDisable;
    Lcd_Init(&LcdConfig);
}

///< LCD 初始化
void AppLcdInit(void)
{
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

    LcdConfig.LcdBiasSrc = LcdExtCap;                               ///< 电容分压模式，需要外部电路配合
    LcdConfig.LcdDuty    = LcdDuty4;                                ///< 1/4 duty
    LcdConfig.LcdBias    = LcdBias3;                                ///< 1/3 BIAS
    LcdConfig.LcdCpClk   = LcdClk2k;                                ///< 电压泵时钟频率选择2kHz
    LcdConfig.LcdScanClk = LcdClk128hz;                             ///< LCD扫描频率选择128Hz
    LcdConfig.LcdMode    = LcdMode0;                                ///< 选择模式0
    LcdConfig.LcdClkSrc  = LcdRCL;                                  ///< LCD时钟选择RCL
    LcdConfig.LcdEn      = LcdEnable;                               ///< 使能LCD模块
    Lcd_Init(&LcdConfig);

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

void AppLcdSetSymbol(enLcdSymbolType_t type, boolean_t display)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    sAppLcdSetSymbol(pstcLcdDisplayCfg, type, display);
}


void AppLcdSetBuzzer(boolean_t display)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    sAppLcdSetSymbol(pstcLcdDisplayCfg, BUZZER_SYM, display);
    return;
}

/* left_v 剩余电量(0格，1格，2格，3格) */
void AppLcdSetBattery(boolean_t display, uint8_t level)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    sAppLcdSetSymbol(pstcLcdDisplayCfg, BAT1_SYM, FALSE);
    sAppLcdSetSymbol(pstcLcdDisplayCfg, BAT2_SYM, FALSE);
    sAppLcdSetSymbol(pstcLcdDisplayCfg, BAT3_SYM, FALSE);

    sAppLcdSetSymbol(pstcLcdDisplayCfg, BATTERY_SYM, display);

    if (level >= BAT_LVL_LOW)
        sAppLcdSetSymbol(pstcLcdDisplayCfg, BAT3_SYM, display);

    if (level >= BAT_LVL_NRM)
        sAppLcdSetSymbol(pstcLcdDisplayCfg, BAT2_SYM, display);

    if (level >= BAT_LVL_HI)
        sAppLcdSetSymbol(pstcLcdDisplayCfg, BAT1_SYM, display);

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

void AppLcdSetLogIndex(uint8_t icon, int16_t index)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;

    pstcLcdDisplayCfg->bLogTempDis = TRUE;
    pstcLcdDisplayCfg->LogIndex = index;
    sAppLcdSetSymbol(pstcLcdDisplayCfg, LOG_SYM, icon);

    pstcLcdDisplayCfg->bLogChanged = TRUE;
}

void AppLcdClearLogIndex(void)
{
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;
    pstcLcdDisplayCfg->LogIndex = -1;
    pstcLcdDisplayCfg->bLogTempDis = FALSE;
    pstcLcdDisplayCfg->bLogRawNum = FALSE;
    sAppLcdSetSymbol(pstcLcdDisplayCfg, LOG_SYM, FALSE);
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
void AppLcdDisplayUpdate(uint32_t delay_ms)
{
    volatile uint8_t *pu8LcdRam;
    uint8_t i = 0;
    stc_lcd_display_cfg_t *pstcLcdDisplayCfg = &gstcLcdDisplayCfg;

    //AppLcdDisplayClear();
    pu8LcdRam = (volatile uint8_t *)(&M0P_LCD->RAM0);
       
    /* temp display */
    if (pstcLcdDisplayCfg->bTempChanged)
    {
        sAppLcdNoDisplayNumber(pu8LcdRam, 4);
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
                pu8LcdRam, pstcLcdDisplayCfg->bRaw_digits_num);
        }
        pstcLcdDisplayCfg->bTempChanged = FALSE;
    }

    if (pstcLcdDisplayCfg->bLogChanged)
    {
        /* log index display */
        sAppLcdNoDisplayLog(pstcLcdDisplayCfg);
        if (pstcLcdDisplayCfg->LogIndex >= 0)
            sAppLcdDisplayLogIndex(pstcLcdDisplayCfg);
 
        pstcLcdDisplayCfg->bLogChanged = FALSE;
    }

    /* symbols display */
    for (i = 0; i < MAX_SYM; ++i)
    {
        sAppLcdDisplaySymbol(pu8LcdRam, i, pstcLcdDisplayCfg->Sym_display[i]);
    }

    if (delay_ms)
        delay1ms(delay_ms);
    
    return;
}

void AppLcdClearAll(void)
{
    memset(&gstcLcdDisplayCfg, 0, sizeof(gstcLcdDisplayCfg));
    AppLcdClearTemp();
    AppLcdClearLogIndex();
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
        delay1ms(200);
    }
}

#define LCD_DEBUG 0
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
        AppLcdDisplayUpdate(500);

        AppLcdSetRawNumber(-10, FALSE, 2);
        AppLcdDisplayUpdate(500);
        
        AppLcdSetRawNumber(-10, TRUE, 3);
        AppLcdDisplayUpdate(500);

        AppLcdSetRawNumber(-10, TRUE, 4);
        AppLcdDisplayUpdate(500);

        AppLcdSetRawNumber(-100, TRUE, 4);
        AppLcdDisplayUpdate(500);

        AppLcdSetBattery(TRUE, 0);
        AppLcdDisplayUpdate(500);

        AppLcdSetBuzzer(TRUE);
        AppLcdDisplayUpdate(500);

        AppLcdSetCheckMode(Surface, TRUE);
        AppLcdDisplayUpdate(500);

        AppLcdSetCheckMode(Body, TRUE);
        AppLcdDisplayUpdate(500);  

        AppLcdSetTempMode(Celsius, TRUE);
        AppLcdDisplayUpdate(500);

        AppLcdSetTempMode(Fahrenheit, TRUE);
        AppLcdDisplayUpdate(500);

        AppLcdClearAll();
        AppLcdDisplayUpdate(500);
        
        for (i = 0; i < MAX_SYM; ++i)
        {
            AppLcdSetSymbol(i, TRUE);
            AppLcdDisplayUpdate(500);
            AppLcdSetSymbol(i, FALSE);
        }
               
        for (i = 0; i < Str_MAX; ++i)
        {
            AppLcdSetString(i);
            AppLcdDisplayUpdate(500);
        }

        AppLcdClearAll();
        AppLcdDisplayUpdate(1000);
        
        for (i = 0; i < 40; ++i)
        {
            tmp = 0;
            tmp += (i >= 30 ? (i - 30) : 0)*1000;
            tmp += ((i >= 20 && i < 30) ? (i -20) : 0)*100;
            tmp += ((i >= 10 && i < 20) ? (i -10) : 0)*10;
            tmp += (i < 10) ? i:0;

            AppLcdSetTemp(tmp);
            AppLcdSetLogIndex(1, tmp > 100 ? tmp/100 : tmp);

            AppLcdSetTempMode(Celsius, i%2);

					
            AppLcdSetBattery(1, i % 4);

            AppLcdDisplayUpdate(500);
        }
    }
}

#endif

/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

