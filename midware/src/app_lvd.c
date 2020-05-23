#include <stdint.h>
#include "utils.h"

#include "lvd.h"
#include "app.h"
#include "app_data.h"

static uint8_t lvd_level = BAT_LVL_NRM;

static int lvd_v[NUM_BAT_LVLS] = {
        [BAT_LVL_CRIT] = LvdMskTH2_5V,
        [BAT_LVL_LOW]  = LvdMskTH2_6V,
        [BAT_LVL_NRM]  = LvdMskTH2_7V,
        [BAT_LVL_HI]   = LvdMskTH3_0V, // full
};

void AppVolMonitorCfg(int en_lvd_threshold)
{
        stc_lvd_cfg_t stcLvdCfg = { 0 };

        Sysctrl_SetPeripheralGate(SysctrlPeripheralVcLvd, TRUE);

        stcLvdCfg.enAct        = LvdActMskInt;       // 配置触发产生中断
        stcLvdCfg.enInputSrc   = LvdInputSrcMskVCC;  // 配置LVD输入源
        stcLvdCfg.enThreshold  = en_lvd_threshold;   // 配置LVD基准电压
        stcLvdCfg.enFilter     = LvdFilterMskEnable; // 滤波使能
        stcLvdCfg.enFilterTime = LvdFilterMsk28_8ms; // 滤波时间设置
        stcLvdCfg.enIrqType    = LvdIrqMskHigh;      // 中断触发类型

        Lvd_Init(&stcLvdCfg);

        Lvd_EnableIrq();
        Lvd_ClearIrq();
        EnableNvic(LVD_IRQn, IrqLevel3, TRUE);

        Lvd_Enable();
}

void AppVolMonitorInit(void)
{
        // battery level is set HIGH by default, put next level as threshold
        AppVolMonitorCfg(lvd_v[lvd_level]);
}

void Lvd_IRQHandler(void)
{
        Lvd_ClearIrq();

        if (!g_rt)
                return;

        if (g_rt->battery_lvl <= BAT_LVL_CRIT)
                return;

        // make sure battery level in runtime is inited as HIGH
        g_rt->battery_lvl = lvd_level;

        if (g_rt->battery_lvl == BAT_LVL_CRIT) {
                Lvd_Disable();
                return;
        }

        AppVolMonitorCfg(lvd_v[--lvd_level]);

        return;
}
