#include "app.h"
#include "math.h"

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
    float32_t delta = a1 * a1 - 4 * a2 * (a0 - Y);
    if(delta >= 0) {
        float32_t X1, X2;
        float32_t N = sqrt(delta);
        X1 = (-a1 - N) / a2 / 2;
        X2 = (-a1 + N) / a2 / 2;

        printf("%f %f\r\n", X1, X2);
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
    fTemp = 10 + iIndex; //stcInfTherBoardPara.f32BlackBodyTempL
    return fTemp;
}

/**
 *******************************************************************************
 ** \brief NTC   环境温度获取
 ** \param [in]  u32AdcNtcHCode Ntc H ADC采样值
 ** \param [in]  u32AdcNtcLCode Ntc L ADC采样值

 ** \retval      Ok         黑体温度
 ******************************************************************************/
float32_t _NNA_NtcTempGet(uint32_t u32AdcNtcHCode, uint32_t u32AdcNtcLCode)
{
    uint32_t uRa;
    uint32_t fNtcRL = stcInfTherBoardPara.u32NtcRL;

    // Ra-T = Vra / VnL * RL
    //< (3090 - 960) / 960 * 51000 = 119k
    uRa = (float32_t)(u32AdcNtcHCode - u32AdcNtcLCode) / u32AdcNtcLCode  * fNtcRL;

    // 算法选择
    if(0) {
        // 查表
        return TNNA_TempNtcFind(uRa);
    } else {
        return TNNA_Fitting(284148.615, -9890.25670, 102.357699, uRa, TRUE);
    }
}

static float32_t TNNA_TempVirFind(float32_t fTempEnv, int32_t i32VirAdc)
{
    int iVirIndex;
    int iIndex = fTempEnv - 10;

    iVirIndex = TNNA_BSearch(i32TempVirTable[iIndex], ARRAY_SIZE(i32TempVirTable[0]), i32VirAdc, FALSE);

    // 10-45C
    iVirIndex += 10;

    return iVirIndex;
}

float32_t _NNA_BlackBodyTempGet(float32_t fTempEnv, uint32_t u32VirAdcCode, boolean_t bMarkEn)
{
    float32_t iVirValue;

    // Vi = (Vo - Vbias) * Ri / (Ri:2K + Rr:680K) --- uV
    iVirValue = ((u32VirAdcCode - 1400) * 1000000) / (2000 + 680000) * 2000;

    // 算法
    if(0) {
        // 查表找出环温对应的 Vir-T
        return TNNA_TempVirFind(fTempEnv, iVirValue);
    } else {
        // 拟合
        return TNNA_Fitting(-2420154, 54351.00, 495.0000, iVirValue, FALSE);
    }
}

