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
/** \file inf_config.h
 **
 ** 配置文件.
 **
 **   - 2020-02-08  LuX V1.0.
 **
 ******************************************************************************/

#ifndef __INF_CONFIG_H__
#define __INF_CONFIG_H__

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "ddl.h"

#define ARRAY_SIZE(A) (sizeof(A)/sizeof(A[0]))

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

/******************************************************************************
 * Global definitions
 ******************************************************************************/
#define TEMPNTCDATALEN          (36u)       ///< 固定默认值
#define TEMPVIR_NTCDATALEN      (36u)       ///< 固定默认值
#define TEMPVIRDATALEN          (36u)       ///< 固定默认值

/******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
struct InfTherBoardPara
{
    ///< ADC 参数配置
    uint32_t u32VirAdcVref;             ///< VIR ADC 参考电压(nV)
    uint32_t u32AdcResolMsk;            ///< ADC 有效位(默认 - [0xFFF])
            
    ///< NTC 校准系数       
    uint32_t  u32NtcRL;                 ///< NTC 对地采样电阻RL(Ω)
    
    float32_t f32NtcKL;                 ///< NTC 校准系数(默认 - [0.0])——低温下相对于标准环境温度下黑体温度的系统偏差率
    ///< 例如：目标黑体33℃的情况下，在25℃测得33℃、在15℃下测得30度，则偏差率为[(33-30)/(25-15) = 0.3]
    float32_t f32NtcKH;                 ///< NTC 校准系数(默认 - [0.0])——高温下相对于标准环境温度下黑体温度的系统偏差率
    ///< 例如：目标黑体33℃的情况下，在25℃测得33℃、在35℃下测得34度，则偏差率为[(33-34)/(25-35) = 0.1]
    
    float32_t f32NtcStdTemp;            ///< NTC 标准环境温度——通常为[25.0]℃
            
    ///< VIR 校准系数       
    float32_t f32VirOpGain;             ///< VIR 信号调理OP增益
    float32_t f32VirK;                  ///< VIR 校准系数(默认 - [1.0]，不需要更改)
    
    float32_t f32BlackBodyTempL;        ///< VIR 黑体标定温度L(用于调试测试模式下系统偏差修正，通常为两点标定时标准环境下实测较低的温度值)
    ///< 例如：未校准情况在标准恒定环境下测得37℃黑体的值为[36℃]；
    float32_t f32BlackBodyTempH;        ///< VIR 黑体标定温度H(用于调试测试模式下系统偏差修正，通常为两点标定时标准环境下实测较高的温度值)
    ///< 例如：未校准情况在标准恒定环境下测得42℃黑体的值为[41℃]；
    
    float32_t f32BlackBodyStdTempL;     ///< VIR 黑体目标(理论)两点标定中较低的标准温度L(例如：通常可以是37℃)
    float32_t f32BlackBodyStdTempH;     ///< VIR 黑体目标(理论)两点标定中较高的标准温度H(例如：通常可以是42℃)
    
    uint32_t  u32BlackBodyTempLAddr;    ///< VIR 黑体标定温度L数据地址——本样例默认FLASH地址为：[0xFC00]，如果使用EE，可参照样例自行实现
    ///< 该地址存放的数据为实际温度*100，功能等价于测试模式下的：[f32BlackBodyTempL*100]
    uint32_t  u32BlackBodyTempHAddr;    ///< VIR 黑体标定温度H数据地址——本样例默认FLASH地址为：[0xFE00]，如果使用EE，可参照样例自行实现
    ///< 该地址存放的数据为实际温度*100，等价于测试模式下的：[f32BlackBodyTempH*100]
};

/******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
extern const int32_t    i32TempVirTable[TEMPVIR_NTCDATALEN][TEMPVIRDATALEN];
extern const int32_t    i32TempNtcTable[TEMPNTCDATALEN];
extern struct InfTherBoardPara const stcInfTherBoardPara;

/******************************************************************************
 * Global function prototypes (definition in C source)                        
 ******************************************************************************/


#ifdef __cplusplus
}
#endif

#endif /* __INF_CONFIG_H__ */
/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

