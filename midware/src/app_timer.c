#include <limits.h>
#include <stdint.h>

#include "app.h"
#include "rtc.h"
#include "timer3.h"
#include "sysctrl.h"

#include "app_timer.h"

#define TIMER3_PCLK_DIV         (256)

uint32_t blink_cnt;

static stc_tim3_mode0_cfg_t timer3_reg_cfg;

static uint8_t              timer3_oneshot = 0;
static void                 *timer3_callback_data;
static void                 (*timer3_irq_callback)(void *priv);

// NOTE: do not change the name, weak function
void Tim3_IRQHandler(uint8_t param)
{
    uint16_t cnt;

    if (TRUE == Tim3_GetIntFlag(Tim3UevIrq)) {
        Tim3_ClearIntFlag(Tim3UevIrq);

        if (timer3_irq_callback != NULL) {
            // __disable_irq();
            timer3_irq_callback(timer3_callback_data);
            // __enable_irq();
        }

        if (timer3_oneshot)
            Tim3_M0_Stop();
    }
}

// due to limited resources, we may calc once and save the result
uint16_t timer3_tick_calc(uint32_t ms)
{
    uint32_t pclk = Sysctrl_GetPClkFreq(); // REVIEW: will PCLK change?!
    uint32_t tick_sec = pclk / TIMER3_PCLK_DIV; // div 1000 here will lose precise
    uint32_t tick;

    tick = tick_sec * ms / 1000;

    if (tick > USHRT_MAX)
        return USHRT_MAX;

    return tick;
}

// auto reload mode:
// count from ARR value to 0xffff
// then reset counter to ARR value, restart couting again
// one tick (count) period in sec = PCLK_HZ / DIV
void timer3_set(uint16_t tick,
                uint8_t oneshot,
                void (irq_callback)(void *),
                void *cb_data)
{        
    uint16_t arr_value;

    // prevent existing timer triggering again
    Tim3_M0_Stop();

    arr_value = (0xFFFF - tick);

    timer3_oneshot = oneshot;
    timer3_callback_data = cb_data;
    timer3_irq_callback = irq_callback;

    Tim3_M0_ARRSet(arr_value);
    Tim3_M0_Cnt16Set(arr_value);
    Tim3_ClearIntFlag(Tim3UevIrq);
    Tim3_Mode0_EnableIrq();
    EnableNvic(TIM3_IRQn, IrqLevel3, TRUE);
}

void timer3_start(void)
{
    Tim3_M0_Stop();
    Tim3_M0_Run();
}

void timer3_stop(void)
{
    Tim3_M0_Stop();
}

void timer3_init(void)
{
    Sysctrl_SetPeripheralGate(SysctrlPeripheralTim3, TRUE);

    timer3_reg_cfg.enWorkMode = Tim3WorkMode0;
    timer3_reg_cfg.enCT       = Tim3Timer;
    timer3_reg_cfg.enPRS      = Tim3PCLKDiv256;
    timer3_reg_cfg.enCntMode  = Tim316bitArrMode;
    timer3_reg_cfg.bEnTog     = FALSE;
    timer3_reg_cfg.bEnGate    = FALSE;
    timer3_reg_cfg.enGateP    = Tim3GatePositive;

    Tim3_Mode0_Init(&timer3_reg_cfg);
}