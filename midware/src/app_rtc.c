
#include "app.h"
#include "app_rtc.h"
#include "app_data.h"
#include "app_key.h"

#include "fsm.h"

uint32_t g_jiffies = 0;

static uint32_t last_feed = 0;

///< 定时关机模块初始化
void AppRtcInit(void)
{
        stc_rtc_initstruct_t RtcInitStruct;
        stc_rtc_alarmtime_t RtcAlmStruct;

        DDL_ZERO_STRUCT(RtcInitStruct);                      //变量初始值置零
        DDL_ZERO_STRUCT(RtcAlmStruct);

        Sysctrl_ClkSourceEnable(SysctrlClkRCL, TRUE);        //使能内部RCL时钟作为RTC时钟
        Sysctrl_SetPeripheralGate(SysctrlPeripheralRtc,TRUE);//RTC模块时钟打开

        RtcInitStruct.rtcAmpm = RtcPm;                       //12小时制
        RtcInitStruct.rtcClksrc = RtcClkRcl;                 //内部低速时钟
        RtcInitStruct.rtcPrdsel.rtcPrdsel = RtcPrds;         //周期中断类型PRDS
        RtcInitStruct.rtcPrdsel.rtcPrds   = Rtc1S;           //不产生周期中断
        RtcInitStruct.rtcTime.u8Second    = 0x00;               //配置RTC时间2019年4月17日10:01:55
        RtcInitStruct.rtcTime.u8Minute    = 0x00;
        RtcInitStruct.rtcTime.u8Hour      = 0x20;
        RtcInitStruct.rtcTime.u8Day       = 0x20;
        RtcInitStruct.rtcTime.u8DayOfWeek = 0x04;
        RtcInitStruct.rtcTime.u8Month     = 0x20;
        RtcInitStruct.rtcTime.u8Year      = 0x20;
        RtcInitStruct.rtcCompen           = RtcCompenDisable;           // 使能时钟误差补偿
        RtcInitStruct.rtcCompValue        = 0;                          //补偿值  根据实际情况进行补偿
        Rtc_Init(&RtcInitStruct);

        RtcAlmStruct.RtcAlarmMinute = 0x01;
        RtcAlmStruct.RtcAlarmHour   = 0x20;
        RtcAlmStruct.RtcAlarmWeek   = 0x7f;                  //从周一到周日，每天10:02:05启动一次闹铃
        Rtc_SetAlarmTime(&RtcAlmStruct);                     //配置闹铃时间
        Rtc_AlmIeCmd(FALSE);                                  //使能闹钟中断

        EnableNvic(RTC_IRQn, IrqLevel3, TRUE);               //使能RTC中断向量
        Rtc_Cmd(TRUE);                                       //使能RTC开始计数
}

void AppRtcFeed(void)
{
        last_feed = g_jiffies;
}

static inline uint32_t AppRtcUpdate(void)
{
        if (is_any_key_pressed()) {
                AppRtcFeed();
                goto out;
        }

        if (!g_cfg)
                goto out;

        if (time_after(g_jiffies, last_feed + g_cfg->sleep_jiffies)) {
                fsm_event_post(&g_fsm, FSM_EVENT_RING_PRIO_HI, FSM_EVENT_SYS_HALT);
        }

out:
        return g_jiffies++;
}

void Rtc_IRQHandler(void)
{
        if (Rtc_GetPridItStatus() == TRUE) {
                Rtc_ClearPrdfItStatus();
                AppRtcUpdate();
        }
}

