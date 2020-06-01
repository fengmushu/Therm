#include "app_afe.h"

static volatile uint32_t u32InitCntData = 0;
/*******************************************************************************
 * PCNT中断服务程序
 ******************************************************************************/
uint32_t PcntGetData(boolean_t reset)
{
    uint32_t uRet = u32InitCntData;

    if(reset)
    {
        u32InitCntData = 0;
    }

    return uRet;
}

static void PcntInt(void)
{
    PCNT_Run(FALSE);

    if (TRUE == PCNT_GetStatus(PCNT_S1E))
    {
        PCNT_ClrStatus(PCNT_S1E);
    }
    if (TRUE == PCNT_GetStatus(PCNT_S0E))
    {
        PCNT_ClrStatus(PCNT_S0E);
    }
    if (TRUE == PCNT_GetStatus(PCNT_BB))
    {
        PCNT_ClrStatus(PCNT_BB);
    }
    if (TRUE == PCNT_GetStatus(PCNT_FE))
    {
        PCNT_ClrStatus(PCNT_FE);
    }
    if (TRUE == PCNT_GetStatus(PCNT_DIR))
    {
        PCNT_ClrStatus(PCNT_DIR);
    }
    if (TRUE == PCNT_GetStatus(PCNT_TO))
    {
        PCNT_ClrStatus(PCNT_TO);
    }
    if (TRUE == PCNT_GetStatus(PCNT_OV))
    {
        PCNT_ClrStatus(PCNT_OV);

        u32InitCntData++;

        PCNT_Parameter(0, AFE_PCNT_RELOAD);

        PCNT_Run(TRUE);
    }
    if (TRUE == PCNT_GetStatus(PCNT_UF))
    {
        PCNT_ClrStatus(PCNT_UF);
    }
}

void AppPcntInit(void)
{
    stc_pcnt_config_t stcConfig;

    DDL_ZERO_STRUCT(stcConfig);

    //PCNT初始化
    stcConfig.bS0Sel = S0P_Noinvert; //极性不取反
    stcConfig.bS1Sel = S1P_Noinvert; //极性不取反
    stcConfig.u8Direc = Direct_Add;  //累加计数
    stcConfig.u8Clk = CLK_Rcl;
    stcConfig.u8Mode = Single_Mode; //单路计量方式
    stcConfig.bFLTEn = TRUE;
    stcConfig.u8FLTDep = 5;
    stcConfig.u8FLTClk = 32;
    stcConfig.bTOEn = TRUE;
    stcConfig.u16TODep = 1000;
    stcConfig.u8IrqStatus = PCNT_OV | PCNT_TO;
    stcConfig.bIrqEn = TRUE;
    stcConfig.pfnIrqCb = PcntInt;

    PCNT_Init(&stcConfig);

    // PCNT_Parameter(0, AFE_PCNT_RELOAD);
}