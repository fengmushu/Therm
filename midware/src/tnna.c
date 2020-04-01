#include "app.h"
#include "math.h"

static float32_t VtT_Paras[3] = {-2420154, 54351.00, 495.00};
static float32_t VtE_Paras[3] = {1589544, -54351.00, -495.00};
static float32_t RaT_Paras[3] = {284148.615, -9890.2567, 102.357699};

static int32_t TNNA_BSearch(const int32_t *uA, uint32_t uLen, int32_t uTarget, boolean_t revert)
{
    int low, upper;

    low = 0;
    upper = uLen - 1;
    while( low <= upper) {
        int m = low + (upper - low) / 2;

        if(uA[m] == uTarget) {return m;}
        if(revert) {
            if(uA[m] > uTarget) {low = m + 1;}
            if(uA[m] < uTarget) {upper = m - 1;}
        } else {
            if(uA[m] < uTarget) {low = m + 1;}
            if(uA[m] > uTarget) {upper = m - 1;}
        }
    }

    return low;
}

///< 
static float32_t TNNA_Fitting(float32_t a0, float32_t a1, float32_t a2, float32_t Y, boolean_t revert)
{
    // 拟合 Y = a0 + a1 * x + a2 * x^2
    float32_t delta = pow(a1, 2) - 4 * a2 * (a0 - Y);
    if(delta >= 0) {
        float32_t X1, X2;
        float32_t N = sqrt(delta);
        X1 = (-a1 - N) / a2 / 2;
        X2 = (-a1 + N) / a2 / 2;

        // printf(" - %f %f\r\n", X1, X2);
        if(revert) {
            return X1;
        } else {
            return X2;
        }
    } else {
        return 0;
    }
}

static float32_t TNNA_TempNtcFind(int32_t uRa)
{
    int iIndex;
    float32_t fTemp;

    iIndex = TNNA_BSearch(i32TempNtcTable, ARRAY_SIZE(i32TempNtcTable), uRa, TRUE);

    //< 10° - 45°
    fTemp = 10 + iIndex; //stTherBoardPara.f32BlackBodyTempL
    return fTemp;
}

/**
 *******************************************************************************
 ** \brief NTC   环境温度获取
 ** \param [in]  u32AdcNtcHCode Ntc H ADC采样值
 ** \param [in]  u32AdcNtcLCode Ntc L ADC采样值

 ** \retval      Ok         黑体温度
 ******************************************************************************/
float32_t NNA_NtcTempGet(uint32_t u32AdcNtcHCode, uint32_t u32AdcNtcLCode)
{
    uint32_t uRa;
    uint32_t fNtcRL = stTherBoardPara.u32NtcRL;
    float32_t fTempBySearch, fTempByFitting;

    // Ra-T = Vra / VnL * RL
    //< (3090 - 960) / 960 * 51000 = 119k
    uRa = (float32_t)(u32AdcNtcHCode - u32AdcNtcLCode) / u32AdcNtcLCode  * fNtcRL;

    // 查表R-Te
    fTempBySearch = TNNA_TempNtcFind(uRa);
    // 拟合R-Te曲线
    fTempByFitting = TNNA_Fitting(RaT_Paras[0], RaT_Paras[1], RaT_Paras[2], uRa, TRUE);

    printf("\t%2.2f %2.2f\r\n", fTempBySearch, fTempByFitting);
    // 二选一
    if(0) {
        return fTempBySearch;
    } else {
        return fTempByFitting;
    }
}

static float32_t TNNA_TempVirFind(float32_t fTempEnv, int32_t i32VirAdc)
{
    int iTempObject;
    int iIndex = fTempEnv - 10;

    // 查表
    iTempObject = TNNA_BSearch(i32TempVirTable[iIndex], ARRAY_SIZE(i32TempVirTable[0]), i32VirAdc, FALSE);

    // 平移
    iTempObject += 10;

    return iTempObject;
}

static float32_t NNA_GetFixupBase(void)
{
    return (AppVirLParaGet() + AppVirHParaGet()) / 2;
}

float32_t NNA_BlackBodyTempGet(float32_t fTempEnv, uint32_t u32VirAdc, boolean_t bMarkEn)
{
    float32_t fVirVolt, fVoltBySearch, fVoltByFitting, fTempEnvFit;
    static float32_t fTempFixup = 0;// = NNA_GetFixupBase();

    ///< 解系统放大系数
    // Vi = (Vo - Vbias) * Ri / (Ri:2K + Rr:680K) --- uV
    fVirVolt = ((u32VirAdc - 1100) * 1000000) / (2000 + 680000) * 2000;

    printf("\t%2.2f uV\r\n", fVirVolt);

    // 查表V-Tt
    fVoltBySearch = TNNA_TempVirFind(fTempEnv, fVirVolt);

    // (Te) -> (V-Tt曲线)
    // c(t) = -0.000495 * t * t - 0.054351 * t + C
    fTempEnvFit = VtE_Paras[1] * fTempEnv - VtE_Paras[2] * pow(fTempEnv, 2) + fTempFixup;
    // U = 0.0004950 * x * x + 0.054351 * x + c(t)
    fVoltByFitting = TNNA_Fitting( fTempEnvFit, VtT_Paras[1], VtT_Paras[2], fVirVolt, FALSE);

    printf("\t%2.2f %2.2f %2.2f\r\n", fVoltBySearch, fVoltByFitting, fTempEnvFit);

    // 二选一
    if(0) {
        // 查表找出环温对应的 Vir-T
        return fVoltBySearch;
    } else {
        // 拟合
        return fVoltByFitting;
    }
}

boolean_t NNA_Calibration(float32_t fTempEnv, float32_t fTempTarget, uint32_t u32VirAdc)
{
    static float32_t fAmp;
    static float32_t fCaLBase, fCaHBase, fTH = 0xFFFFFFFF, fTL = 0xFFFFFFFF, uVAdcH, uVAdcL;;

    float32_t fTx;

    ///< U35 = k * (A*35*35 + B*35 + a*t35*t35 + b*t35 + cbase)
    ///< U42 = k * (A*42*42 + B*42 + a*t42*t42 + b*t42 + cbase)
    ///< m = A*35*35 + B*35 + a*t35*t35 + b*t35
    ///< n = A*42*42 + B*42 + a*t42*t42 + b*t42
    ///< k = (U42-U35) / (n-m)
    ///< cbase = U35/k - m 
    ///< 或者
    ///< cbase = U42/k - n

    ///< k: fAMP, m/n: fTx, Ux: fVirAdcH/L, cbase: fCaH/LBase
    fTx = VtT_Paras[2] * pow(fTempTarget, 2) + VtT_Paras[1] * fTempTarget;
    fTx += VtE_Paras[2] * pow(fTempEnv, 2) + VtE_Paras[1] * fTempEnv;

    if(fTempTarget < 40) {
        fTL = fTx;
        uVAdcL = u32VirAdc * 1000000;
    } else {
        fTH = fTx;
        uVAdcH = u32VirAdc * 1000000;
    }

    if(fTL != 0xFFFFFFFF && fTH != 0xFFFFFFFF && ((fTL - fTH != 0))) {
        fAmp = (uVAdcH - uVAdcL) / (fTH - fTL); // 线性区间放大
        if(fAmp != 0) {
            fCaLBase = uVAdcL / fAmp - fTL;
            fCaHBase = uVAdcH / fAmp - fTH;

			printf("\tAMP: %2.2f %2.2f %2.2f\r\n", fAmp, fCaLBase, fCaHBase);
        }
    }

    // 标定
    // if(0) {
        ///< U = A*x^2 + B*x + c(t)
        ///< c(t) = a*t^2 + b*t + Base

        ///< Base = U - b * t - a * t^2 - B*x - A * x^2
        // fBase = fVoltByFitting - (-54351.00 * fTempEnv) - (-495.00 * fTempEnv * fTempEnv);
        // fBase = fBase - (54351.00 * fFixCurr) - (495.00 * fFixCurr * fFixCurr);
    // }
    return TRUE;
}
