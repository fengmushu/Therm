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
/** \file nna.h
 **
 **  Vir温度算法API.
 **
 **   - 2020-04-21  LuX V1.0.
 **
 ******************************************************************************/

#ifndef __NNA_H__
#define __NNA_H__

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "ddl.h"
#include "inf_config.h"

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
#define NTC_CAL_VALUE   (250)       //(0.1℃)    校准参数NTC标定预设值
#define VIR_CAL_VALUE   (370)       //(0.1℃)    校准参数VIR标定预设值

/******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/******************************************************************************
 * Global function prototypes (definition in C source)                        
 ******************************************************************************/

/**
 *******************************************************************************
 ** \brief NTC   环境温度获取
 ** \param [in]  u32AdcNtcHCode Ntc H ADC采样值
 ** \param [in]  u32AdcNtcLCode Ntc L ADC采样值

 ** \retval      Ok         黑体温度
 ******************************************************************************/
extern float32_t NNA_NtcTempGet(uint32_t u32AdcNtcHCode, uint32_t u32AdcNtcLCode);

/**
 *******************************************************************************
 ** \brief VIR 黑体/物体 温度
 ** \param [in]  f32NtcTemp     Ntc温度
 ** \param [in]  u32AdcCode     Vir ADC采样值
 ** \param [in]  u32AdcBiasCode Vir Bias ADC采样值
 ** \param [in]  fEpsilon       物体表面的 热辐射率

 ** \retval      Ok         黑体温度
 ******************************************************************************/
extern float32_t NNA_SurfaceTempGet(float32_t f32NtcTemp, uint32_t u32VirAdcCode, float32_t fEpsilon);

/**
 *******************************************************************************
 ** \brief VIR 人体温度获取
 ** \param [in]  fSurfaceTemp   表面温度

 ** \retval                         人体温度
 ******************************************************************************/
extern float32_t NNA_HumanBodyTempGet(float32_t fSurfaceTemp);


extern boolean_t NNA_Calibration(float32_t fTempEnv, float32_t fTempTarget, uint32_t u32VirAdc);
//@} // APP Group
   
#ifdef __cplusplus
}
#endif

#endif /* __NNA_H__ */
/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

