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
/** \file app_adc.c
 **
 ** ADC driver API.
 **
 **   - 2017-06-28 Alex    First Version
 **
 ******************************************************************************/

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "adc.h"
#include "app_adc.h"
#include "app.h"
#include "bgr.h"
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

/*****************************************************************************
 * Function implementation - global ('extern') and local ('static')
 *****************************************************************************/
void AppMAdcPowerOn(void)
{
    // Gpio_SetIO(M_ADC_VBIRS_PORT, M_ADC_VBIRS_PIN); 
    Gpio_SetIO(M_ADC_VBIRS_EN_PORT, M_ADC_VBIRS_EN_PIN);
}

void AppMAdcPowerOff(void)
{
    // Gpio_ClrIO(M_ADC_VBIRS_PORT, M_ADC_VBIRS_PIN);
    Gpio_ClrIO(M_ADC_VBIRS_EN_PORT, M_ADC_VBIRS_EN_PIN);
}

///< ADC 顺序扫描(多次采样) 功能配置
void _AppAdcSQRCfg(en_adc_samp_ch_sel_t enAdcSampCh)
{
    stc_adc_sqr_cfg_t          stcAdcSqrCfg;
    
    DDL_ZERO_STRUCT(stcAdcSqrCfg);
        
    ///< 顺序扫描模式功能及通道配置
    ///< 注意：扫描模式下，当配置转换次数为n时，转换通道的配置范围必须为[SQRCH(0)MUX,SQRCH(n-1)MUX]
    stcAdcSqrCfg.bSqrDmaTrig = FALSE;
    stcAdcSqrCfg.enResultAcc = AdcResultAccEnable;
    stcAdcSqrCfg.u8SqrCnt    = 16;
    Adc_SqrModeCfg(&stcAdcSqrCfg);

    Adc_CfgSqrChannel(AdcSQRCH0MUX , enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH1MUX , enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH2MUX , enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH3MUX , enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH4MUX , enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH5MUX , enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH6MUX , enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH7MUX , enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH8MUX , enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH9MUX , enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH10MUX, enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH11MUX, enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH12MUX, enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH13MUX, enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH14MUX, enAdcSampCh);
    Adc_CfgSqrChannel(AdcSQRCH15MUX, enAdcSampCh);

}


///< ADC 模块初始化
void AppMAdcInit(void)
{
    stc_adc_cfg_t              stcAdcCfg;

    DDL_ZERO_STRUCT(stcAdcCfg);
    
    ///< 开启ADC/BGR外设时钟
    Sysctrl_SetPeripheralGate(SysctrlPeripheralAdcBgr, TRUE); 
    
    Bgr_BgrEnable();        ///< 开启BGR
    
    ///< ADC 初始化配置
    stcAdcCfg.enAdcMode         = AdcScanMode;                  ///<采样模式-扫描
    stcAdcCfg.enAdcClkDiv       = AdcMskClkDiv8;                ///<采样分频-1
    stcAdcCfg.enAdcSampCycleSel = AdcMskSampCycle12Clk;         ///<采样周期数-8
    stcAdcCfg.enAdcRefVolSel    = AdcMskRefVolSelInBgr2p5;      ///<参考电压选择-AVDD
    stcAdcCfg.enAdcOpBuf        = AdcMskBufDisable;             ///<OP BUF配置-关
    stcAdcCfg.enInRef           = AdcMskInRefEnable;            ///<内部参考电压使能-关
    stcAdcCfg.enAdcAlign        = AdcAlignRight;                ///<转换结果对齐方式-右
    Adc_Init(&stcAdcCfg);
}

///< ADC通道数据读取
void AppAdcChAvgCodeGet(en_adc_samp_ch_sel_t ch, uint32_t *pu32AdcRestultAcc)
{
    _AppAdcSQRCfg(ch);
    Adc_ClrIrqStatus(AdcMskIrqSqr);
    
    Adc_SQR_Start();
    
    while(FALSE == Adc_GetIrqStatus(AdcMskIrqSqr)){;}
       
    *pu32AdcRestultAcc = Adc_GetAccResult();
    *pu32AdcRestultAcc = (*pu32AdcRestultAcc + 0x8u)>>4;
    
    Adc_SQR_Stop();

    Adc_ClrAccResult();
    Adc_ClrIrqStatus(AdcMskIrqSqr);
}

///< 环境温度采样值L获取(采样16次取平均)
void AppAdcNtcLAvgCodeGet(uint32_t *pu32AdcRestultAcc)
{    
    AppAdcChAvgCodeGet(AdcExInputCH1, pu32AdcRestultAcc);
}

///< 环境温度采样值L获取(采样16次取平均)
void AppAdcNtcHAvgCodeGet(uint32_t *pu32AdcRestultAcc)
{    
    AppAdcChAvgCodeGet(AdcExInputCH2, pu32AdcRestultAcc);
}

///< 红外温度采样值获取(采样16次取平均)
void AppAdcVirAvgCodeGet(uint32_t *pu32AdcRestultAcc)
{    
    AppAdcChAvgCodeGet(AdcExInputCH0, pu32AdcRestultAcc);
}

///< VBias采样值获取(采样16次取平均)
// void AppAdcVBiasAvgCodeGet(uint32_t *pu32AdcRestultAcc)
// {    
//         AppAdcChAvgCodeGet(AdcExInputCH11, pu32AdcRestultAcc);
// }

//@} // AdcGroup


/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

