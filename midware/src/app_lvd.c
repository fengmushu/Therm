#include <stdint.h>
#include "utils.h"

#include "lvd.h"
#include "app.h"
#include "app_data.h"

int bat_lvd_thres[NUM_BAT_LVLS] = {
    [BAT_LVL_CRIT] = LvdMskTH2_5V,
    [BAT_LVL_LOW]  = LvdMskTH2_6V,
    [BAT_LVL_NRM]  = LvdMskTH2_7V,
    [BAT_LVL_HI]   = LvdMskTH2_9V,
};

void AppVolMonitorCfg(int en_lvd_threshold)
{
    stc_lvd_cfg_t stcLvdCfg;

    DDL_ZERO_STRUCT(stcLvdCfg);     //变量清0

    Sysctrl_SetPeripheralGate(SysctrlPeripheralVcLvd, TRUE);    //开LVD时钟

    stcLvdCfg.enAct        = LvdActMskInt;              ///< 配置触发产生中断
    stcLvdCfg.enInputSrc   = LvdInputSrcMskVCC;         ///< 配置LVD输入源
    stcLvdCfg.enThreshold  = en_lvd_threshold;          ///< 配置LVD基准电压
    stcLvdCfg.enFilter     = LvdFilterMskEnable;        ///< 滤波使能
    stcLvdCfg.enFilterTime = LvdFilterMsk28_8ms;        ///< 滤波时间设置
    stcLvdCfg.enIrqType    = LvdIrqMskHigh;             ///< 中断触发类型
    Lvd_Init(&stcLvdCfg);
    
    ///< 中断开启
    Lvd_EnableIrq();
    Lvd_ClearIrq();
    EnableNvic(LVD_IRQn, IrqLevel3, TRUE);              ///< NVIC 中断使能
    
    ///< LVD 模块使能
    Lvd_Enable();
}

void AppVolMonitorInit(void)
{
    AppVolMonitorCfg(bat_lvd_thres[BAT_LVL_HI]);
}

void Lvd_IRQHandler(void)
{
    Lvd_ClearIrq();

    if (!g_rt)
        return;

    // make sure battery level in runtime is inited as HIGH
    g_rt->battery_lvl--;

    if (g_rt->battery_lvl == BAT_LVL_CRIT) {
        Lvd_Disable();
        return;
    }

    AppVolMonitorCfg(bat_lvd_thres[g_rt->battery_lvl]);
}
