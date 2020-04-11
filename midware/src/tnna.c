#include "app.h"
#include "math.h"

#include "app_data.h"

#define USE_FITTING 1
#define VRA_INIT (0)

#define USE_AUTO_SENSOR

static float32_t VtT_Paras[3] = {0, 0, 0};
typedef struct {
    uint16_t uSensorType;

    float32_t   VtE0_Paras[3];
    float32_t   VtE1_Paras[3];
    float32_t   VtE2_Paras[3];

    int32_t     RaT_Paras_P;
    float32_t   RaT_Paras_F;
    float32_t   RaT_Paras[3];
} sensor_t;

static const sensor_t Sensors[en_sensor_max] = {
    {
        .uSensorType = 1875, //1875
        .VtE2_Paras = {495.00, 0, 0},
        .VtE1_Paras = {54351.00, 0, 0},
        .VtE0_Paras = {0, -54351.00, -495.00},
        .RaT_Paras_P = 2,
        .RaT_Paras_F = 1,
        .RaT_Paras = {19263, -4305.730, 42.1632},
    },
    {
        .uSensorType = 55, //F55
        .VtE2_Paras = {254.60, 0, 0},
        .VtE1_Paras = {56371.71, -169.35, 0},
        .VtE0_Paras = {-89766.89, -46033.28, -308.06},
        .RaT_Paras_P = 2,
        .RaT_Paras_F = 1,
        .RaT_Paras = {18798.8, -4247.53, 40.9328},
    },
    {
        .uSensorType = 01, //MTS01
        .VtE2_Paras = {319.61, 0, 0},
        .VtE1_Paras = {37659.74, 0, 0},
        .VtE0_Paras = {27624.49, -40557.05, -271.40},
        //0 = 0.1479x2 - 11.97x + 305.12 - R
        .RaT_Paras_P = 0,
        .RaT_Paras_F = -1,
        .RaT_Paras = {295.51, -10.8, 0.1184},
    },
    {
        .uSensorType = 7, //B7
        .VtE2_Paras = {750, 0, 0},
        .VtE1_Paras = {82350, 0, 0},
        .VtE0_Paras = {0, -82350, -750},
        //0 = 0.144x2 - 11.784x + 303.19 -R
        .RaT_Paras_P = 0,
        .RaT_Paras_F = -1,
        .RaT_Paras = {303.19, -11.784, 0.144},
    },
};

static const sensor_t *gSensor = &Sensors[DEFAULTL_SENSOR];

#ifndef USE_FITTING
static int32_t TNNA_BSearch(const int32_t *uA, uint32_t uLen, int32_t uTarget, boolean_t revert)
{
    int low, upper;

    low = 0;
    upper = uLen - 1;
    while (low <= upper)
    {
        int m = low + (upper - low) / 2;

        if (uA[m] == uTarget)
        {
            return m;
        }
        if (revert)
        {
            if (uA[m] > uTarget)
            {
                low = m + 1;
            }
            if (uA[m] < uTarget)
            {
                upper = m - 1;
            }
        }
        else
        {
            if (uA[m] < uTarget)
            {
                low = m + 1;
            }
            if (uA[m] > uTarget)
            {
                upper = m - 1;
            }
        }
    }

    return low;
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

///< 查表
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

#else
///<
static float32_t TNNA_Fitting(float32_t a0, float32_t a1, float32_t a2, float32_t Y, boolean_t revert)
{
    // 拟合 Y = a0 + a1 * x + a2 * x^2
    float32_t delta = pow(a1, 2) - 4 * a2 * (a0 - Y);
    if (delta >= 0)
    {
        float32_t X1, X2;
        float32_t N = sqrt(delta);
        X1 = (-a1 - N) / a2 / 2;
        X2 = (-a1 + N) / a2 / 2;

        DBG_PRINT("\t- %f %f\r\n", X1, X2);
        // AppLcdSetTemp((uint32_t)(X1 * 10));
        // AppLcdSetLogTemp((uint32_t)(X2 * 10), 0);
        // AppLcdDisplayUpdate(1000);
        if (revert)
        {
            return X1;
        }
        else
        {
            return X2;
        }
    }
    else
    {
        return 0;
    }
}
#endif //USE_FITTING

/**
 *******************************************************************************
 ** \brief NTC   环境温度获取
 ** \param [in]  u32AdcNtcHCode Ntc H ADC采样值
 ** \param [in]  u32AdcNtcLCode Ntc L ADC采样值

 ** \retval      Ok         黑体温度
 ******************************************************************************/
float32_t NNA_NtcTempGet(uint32_t u32AdcNtcHCode, uint32_t u32AdcNtcLCode, uint32_t* uRa)
{
    uint32_t fNtcRL = 51000;
    float32_t RaT_params[3];

    memcpy(RaT_params, gSensor->RaT_Paras, sizeof(gSensor->RaT_Paras));

    // Ra-T = Vra / VnL * RL
    //< (3090 - 960) / 960 * 51000 = 119k
    *uRa = (float32_t)(u32AdcNtcHCode - u32AdcNtcLCode) / u32AdcNtcLCode * fNtcRL;

#ifndef USE_FITTING
    // 查表R-Te
    return TNNA_TempNtcFind(*uRa);
#else
    // 拟合R-Te曲线
    RaT_params[gSensor->RaT_Paras_P] = RaT_params[gSensor->RaT_Paras_P] + gSensor->RaT_Paras_F * ((float32_t)(*uRa) / 1000);
    return TNNA_Fitting(RaT_params[0], RaT_params[1], RaT_params[2], 0, (gSensor->RaT_Paras_P == 0));
#endif //USE_FITTING
}

float32_t NNA_SurfaceTempGet(CalData_t *pCal, float32_t fTempEnv, uint32_t u32VirAdc, float32_t fEpsilon)
{
    float32_t fVirVolt, fVoltBySearch, fVoltByFitting;
    float32_t fAmp = pCal->fAmp;
    float32_t fTempFixup = pCal->fCalBase;

    DBG_PRINT("# %2.2f uV\r\n", (float32_t)u32VirAdc * 1000000);

    if (0 == fAmp)
    {
        return 0;
    }
    else
    {
        // DBG_PRINT("\t* %2.2f %2.2f\r\n", fAmp, fTempFixup);
    }

    ///< 解系统放大系数
    // Vi = (Vo - Vbias) * Ri / (Ri:2K + Rr:680K) --- uV
    fVirVolt = ((u32VirAdc)*1000000) / fAmp / fEpsilon;

    DBG_PRINT("\t$ %2.2f uV\r\n", fVirVolt);

#ifndef USE_FITTING
    // 查表V-Tt
    return TNNA_TempVirFind(fTempEnv, fVirVolt);
#else
    // (Te) -> (V-Tt曲线)
    // c(t) = -0.000495 * t * t - 0.054351 * t + C
    //C(t)
    VtT_Paras[0] = gSensor->VtE0_Paras[0] + gSensor->VtE0_Paras[1] * fTempEnv + gSensor->VtE0_Paras[2] * pow(fTempEnv, 2) + fTempFixup;

    //B(t)
    VtT_Paras[1] = gSensor->VtE1_Paras[0] + gSensor->VtE1_Paras[1] * fTempEnv + gSensor->VtE1_Paras[2] * pow(fTempEnv, 2);

    //A(t)
    VtT_Paras[2] = gSensor->VtE2_Paras[0] + gSensor->VtE2_Paras[1] * fTempEnv + gSensor->VtE2_Paras[2] * pow(fTempEnv, 2);

    // U = 0.0004950 * x * x + 0.054351 * x + c(t)
    return TNNA_Fitting(VtT_Paras[0], VtT_Paras[1], VtT_Paras[2], fVirVolt, FALSE);
#endif //USE_FITTING
}

uint16_t NNA_SensorGet(void)
{
    return gSensor->uSensorType;
}

en_sensor_t NNA_SensorGetIndex(void)
{
    return (en_sensor_t)(gSensor - &Sensors[0]);
}

boolean_t NNA_SensorSet(en_sensor_t uSensorType)
{
    if(uSensorType >= en_sensor_max) {
        return FALSE;
    }

    gSensor = &Sensors[uSensorType];
    return TRUE;
}

void NNA_CalInit(CalData_t *pCal)
{
    memset(pCal, 0, sizeof(CalData_t));

    pCal->fTH = pCal->fTL = VRA_INIT;
    pCal->u8HumanFix = 29;
}

boolean_t NNA_Calibration(
    CalData_t *pCal,
    float32_t fTempEnv,
    float32_t fTempTarget,
    float32_t *fTempGet,
    uint32_t u32VirAdc)
{
    float32_t fTx, fCaLBase, fCaHBase;

    float32_t fAmp = pCal->fAmp, fTH = pCal->fTH, fTL = pCal->fTL;
    float32_t uVAdcH = pCal->uVAdcH, uVAdcL = pCal->uVAdcL;

    if (fAmp)
    {
        return TRUE;
    }

    ///< U35 = k * (A*35*35 + B*35 + a*t35*t35 + b*t35 + cbase )
    ///< U42 = k * (A*42*42 + B*42 + a*t42*t42 + b*t42 + cbase )
    ///< m = A*35*35 + B*35 + a*t35*t35 + b*t35 + cbase
    ///< n = A*42*42 + B*42 + a*t42*t42 + b*t42 + cbase
    ///< k = (U42-U35) / (n-m)
    ///< cbase = U35/k - m
    ///< 或者
    ///< cbase = U42/k - n

    // U35 = K * (A(35)*35*35 + B(35)* 35 + C(35) + cbase )
    // U42 = K * (A(42)*42*42 + B(42)* 42 + C(42) + cbase )
    // m = A(35)*35*35 + B(35)* 35 + C(35)
    // n = A(42)*42*42 + B(42)* 42 + C(42)
    // k = (U42-U35) / (n-m)
    ///< cbase = U35/k - m
    ///< 或者
    ///< cbase = U42/k - n

    ///< k: fAMP, m/n: fTx, Ux: fVirAdcH/L, cbase: fCaH/LBase
    VtT_Paras[0] = gSensor->VtE0_Paras[0] + gSensor->VtE0_Paras[1] * fTempEnv + gSensor->VtE0_Paras[2] * pow(fTempEnv, 2); //C35 or C42

    //B(t)
    VtT_Paras[1] = gSensor->VtE1_Paras[0] + gSensor->VtE1_Paras[1] * fTempEnv + gSensor->VtE1_Paras[2] * pow(fTempEnv, 2); //B35 or B42

    //A(t)
    VtT_Paras[2] = gSensor->VtE2_Paras[0] + gSensor->VtE2_Paras[1] * fTempEnv + gSensor->VtE2_Paras[2] * pow(fTempEnv, 2); //A35 or A42

    fTx = VtT_Paras[2] * pow(fTempTarget, 2) + VtT_Paras[1] * fTempTarget + VtT_Paras[0];
    ///< 返回目标温度实测值
    *fTempGet = fTx;

    if (fTempTarget < 40)
    {
        pCal->fTL = fTL = fTx;
        pCal->uVAdcL = uVAdcL = u32VirAdc * 1000000;
    }
    else
    {
        pCal->fTH = fTH = fTx;
        pCal->uVAdcH = uVAdcH = u32VirAdc * 1000000;
    }

    DBG_PRINT("\tInput: %f, %f, %f\r\n", fAmp, fTL, fTH);
    if (fTL != VRA_INIT && fTH != VRA_INIT && ((fTL - fTH != 0)))
    {
        // 线性区间放大
        fAmp = (uVAdcH - uVAdcL) / (fTH - fTL);
        if (fAmp >= 100 && fAmp <= 1800)
        {
            fCaLBase = uVAdcL / fAmp - fTL;
            fCaHBase = uVAdcH / fAmp - fTH;

            pCal->fAmp = fAmp;
            pCal->fCalBase = (fCaHBase + fCaLBase) / 2;

            AppLcdSetTemp((uint32_t)(fAmp * 10));
            AppLcdDisplayUpdate(400);

            DBG_PRINT("\tAMP: %2.2f L: %2.2f H: %2.2f %2.2f %2.2f\r\n", fAmp, fTL, fTH, fCaLBase, fCaHBase);
        }
        else
        {
            AppLcdSetTemp((uint32_t)(fAmp * 10));
            AppLcdDisplayUpdate(3000);

            DBG_PRINT("\tAMP-OF: %2.2f, %2.2f\r\n", fAmp, fTx);
            return FALSE;
        }
    }
    return TRUE;
}

float32_t NNA_HumanBodyTempGet(CalData_t *pCal, float32_t fNtcTemp, float32_t fSkinTemp)
{
    float32_t cal_tweak;

    if (!g_cfg)
    {
        cal_tweak = 0.0f;
    }
    else
    {
        cal_tweak = (float32_t)(g_cfg->body_cal_tweak) / 10;
    }

    if (fSkinTemp < 28)
    {
        return 0; //Lo
    }

    if (fSkinTemp > 42)
    {
        return 100; //Hi
    }

    if (fNtcTemp < 20)
    {
        if (fSkinTemp <= 35.2)
        {
            fSkinTemp += 1;
        }
    }
    else
    {
        if (fSkinTemp <= 31)
        {
            fSkinTemp += 1;
        }
    }

    if (fSkinTemp <= 36.2)
    {
        return pCal->u8HumanFix + cal_tweak + 0.22 * fSkinTemp;
    }

    if (fSkinTemp <= 38)
    {
        return fSkinTemp + 0.8;
    }

    return fSkinTemp + 1.3;
}