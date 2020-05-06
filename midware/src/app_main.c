#include <stdint.h>
#include <stdlib.h>

#include "fsm.h"
#include "app.h"
#include "app_lcd.h"
#include "app_i2c.h"
#include "app_key.h"
#include "app_data.h"
#include "app_timer.h"
#include "app_main.h"
#include "app_rtc.h"
#include "app_factory.h"

#define BLINK_PERIOD_CNT                (1 << 1)

#define BEEP_SCAN_DONE_MS               (100)

static uint8_t blink_cnt;

fsm_state_t state_main_enter(fsm_node_t *node, fsm_event_t event)
{
    uint8_t scan_mode = scan_mode_runtime_update();
    int16_t last_result = g_rt->scan_result[scan_mode];

    switch (event) {
    case FSM_EVENT_SCAN_DONE:
        AppRtcFeed();
        break;

    default:
        AppLcdClearAll();

        break;
    }

    return node->state;
}

static inline int16_t body_beep_alarm(void)
{
    static const int16_t a_delay = 100;
    int16_t ret = 0;

    for (uint8_t i = 0; i < 5; i++) {
        beep_on();
        delay1ms(a_delay);
        ret += a_delay;

        beep_off();
        delay1ms(a_delay);
        ret += a_delay;
    }

    return ret;
}

fsm_state_t state_main_proc(fsm_node_t *node, fsm_event_t *out)
{
    fsm_state_t next = node->state;
    uint8_t scan_show = g_rt->scan_show;
    uint8_t scan_mode = g_rt->scan_mode_last;
    uint8_t read_idx = g_rt->read_idx[scan_mode];

    // by testing, say 5 at least to stabilize lcd
    const int16_t lcd_budget_ms = 20;
    const int16_t key_budget_ms = 150;
    int16_t delay_budget = lcd_budget_ms;

    int16_t big_number;
    int16_t log_number;

    // reset color for all not defined patterns
    AppLedEnable(LedGreen);

    // blinking
    if (g_rt->battery_low) {
        uint8_t duty;

        duty = blink_cnt & GENMASK(5, 4);
        duty >>= 4;
    }

delay:
    // share delay for key, lcd, beep
    if (delay_budget > 0)
        delay1ms(delay_budget);

out:
    g_rt->scan_done = 0;
    g_rt->scan_mode_last = scan_mode_runtime_update();

    blink_cnt++;

    return next;
}

void state_main_exit(fsm_node_t *node, fsm_event_t event)
{
    timer3_stop();
}

fsm_state_t state_main_release_minus(fsm_node_t *node, fsm_event_t event, void *data)
{
    return node->state;
}

fsm_state_t state_main_release_plus(fsm_node_t *node, fsm_event_t event, void *data)
{
    return node->state;
}

fsm_state_t state_main_scan_mode_switch(fsm_node_t *node, fsm_event_t event, void *data)
{
    // to display history
    uint8_t scan_mode = scan_mode_runtime_update();

    g_rt->scan_show = 0;

    return node->state;
}

static void fn_hold_timer(void *data)
{
    fsm_event_post(&g_fsm, FSM_EVENT_RING_PRIO_HI, FSM_EVENT_IRQ_TIMER3);
}

fsm_state_t state_main_press_fn(fsm_node_t *node, fsm_event_t event, void *data)
{
    timer3_set(TIM3_PCLK_4M256D_2SEC, 1, fn_hold_timer, NULL);
    timer3_start();

    return node->state;
}

fsm_state_t state_main_release_fn(fsm_node_t *node, fsm_event_t event, void *data)
{
    timer3_stop();

    return node->state;
}
