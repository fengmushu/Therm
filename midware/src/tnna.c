#include "app.h"
#include "math.h"

static int bsearch(const int32_t *uA, uint32_t uLen, int32_t uTarget, boolean_t revert)
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

static float32_t NNA_TempNtcFind(int32_t uRa)
{
    int iIndex;
    float32_t fTemp;

    iIndex = bsearch(i32TempNtcTable, ARRAY_SIZE(i32TempNtcTable), uRa, TRUE);

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
        return NNA_TempNtcFind(uRa);
    } else {
        // 拟合 Y = a0 + a1 * x + a2 * x^2
        float32_t a0 = 284148.615, a1 = -9890.25670,  a2 = 102.357699;
        // Ra : c(t) = -0.000495 * t * t - 0.054351 * t
        // b^2 - 4 * a * (c - uRa) => X = a1 * a1 - 4 * a2 * (a0 - Ra)
        // uRa = - b +/- "X" / 2 / a2;

        float32_t X = a1 * a1 - 4 * a2 * (a0 - uRa);
        if(X >= 0) {
            float32_t X1, X2;

            X = sqrt(X);
            X1 = (-a1 - X) / 2 / a2;
            X2 = (-a1 + X) / 2 / a2;

            ///< 曲线斜率向上, 取第一个解
            // printf("%f %f\r\n", X1, X2);
            return X1;
        } else {
            return -1;
        }
    }
}

static float32_t NNA_TempVirFind(float32_t fTempEnv, uint32_t u32VirAdc)
{
    int iVirIndex;
    int iIndex = fTempEnv - 10;

    iVirIndex = bsearch(i32TempVirTable[iIndex], ARRAY_SIZE(i32TempVirTable[0]), u32VirAdc, FALSE);

    // 10-45C
    iVirIndex += 10;

    return iVirIndex;
}

float32_t _NNA_BlackBodyTempGet(float32_t fTempEnv, uint32_t u32VirAdcCode, boolean_t bMarkEn)
{
    float32_t fVirT;
    uint32_t uVirValue = u32VirAdcCode; //FIXME:~

    // 找出环温对应的 Vir-T
    fVirT = NNA_TempVirFind(fTempEnv, uVirValue);

    return fVirT;
}

