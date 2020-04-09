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

#define BLINK_PERIOD_CNT                (1 << 1)

#define BEEP_SCAN_DONE_MS               (100)

static uint8_t blink_cnt;

fsm_state_t state_main_enter(fsm_node_t *node, fsm_event_t event)
{
    uint8_t scan_mode = scan_mode_runtime_update();
    int16_t last_result = g_rt->scan_result[scan_mode];
    scan_log_t *scan_log = &g_scan_log[scan_mode];

    switch (event) {
    case FSM_EVENT_SCAN_DONE:
        AppRtcFeed();

        g_rt->scan_show = 1; // to keep big number showing last scan result
        g_rt->scan_done = 1;
        g_rt->scan_mode_last = scan_mode; // make sure next lcd display is matched to log

        if (is_temp_valid(&g_temp_thres[scan_mode], last_result)) {
            // last_write++ in scan_log_write()
            scan_log_write(scan_log, last_result);
            g_rt->read_idx[scan_mode] = scan_log->last_write;
        }

        break;

    default: // display last write result
        g_rt->scan_show = 0;
        g_rt->scan_done = 0;
        g_rt->scan_mode_last = scan_mode;
        g_rt->read_idx[scan_mode] = scan_log->last_write;

        AppLcdClearAll();

        break;
    }

    return node->state;
}

static inline int16_t body_beep_alarm(void)
{
    static const int16_t a_delay = 150;
    int16_t ret = 0;

    for (uint8_t i = 0; i < 4; i++) {
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
    int16_t big_number;
    int16_t log_number;
    int16_t burst_delay = 100;

    // reset color for all not defined patterns
    AppLedEnable(LedGreen);

    AppLcdSetLock(FALSE);
    AppLcdSetBuzzer(g_cfg->beep_on);
    AppLcdSetTempMode(g_cfg->temp_unit, TRUE);
    AppLcdSetCheckMode(g_rt->scan_mode, TRUE);

    // blinking
    if (g_rt->battery_low) {
        uint8_t duty;

        duty = blink_cnt & GENMASK(5, 4);
        duty >>= 4;

        AppLcdSetBattery(FALSE);

        if (duty == 0x00 || duty == 0x01) // duty 50%
            AppLcdSetBattery(TRUE);
    }

    // read_idx should have synced to write_idx in enter() if comes from scan
    log_number = scan_log_read(&g_scan_log[scan_mode], read_idx);

    AppLcdSetLogTemp(C2F_by_setting(log_number), lcd_show_idx(read_idx));

    // user is viewing log, show big number as log number
    big_number = log_number;

    if (scan_show) {
        big_number = g_rt->scan_result[scan_mode];

        if (big_number < g_temp_thres[scan_mode].underflow) {
            AppLedEnable(LedOrange);
            AppLcdSetString(Str_LO);
            goto lcd_update; // jump out
        } else if (big_number > g_temp_thres[scan_mode].overflow) {
            AppLedEnable(LedRed);
            AppLcdSetString(Str_HI);
            goto lcd_update; // jump out
        }
    }

    //
    // surface mode is always green, and LED is set green already
    //
    if (scan_mode == SCAN_BODY) {
        // currently, body_alarm_C can be smaller than BODY_FEVER_LOW
        if (big_number >= g_cfg->body_alarm_C) {
            AppLedEnable(LedRed);
        } else if (big_number >= BODY_FEVER_LOW) {
            AppLedEnable(LedOrange);
        }
    }

    AppLcdSetRawNumber(C2F_by_setting(big_number), TRUE, 2);

lcd_update:
    AppLcdDisplayUpdate(30);

    // scan_done will be oneshot after scan done
    if (g_rt->scan_done) {
        if (g_cfg->beep_on) {
            if (scan_mode == SCAN_BODY && big_number >= g_cfg->body_alarm_C) {
                burst_delay -= body_beep_alarm();
            } else {
                // user may wanna release trigger after hearing beep
                // so put a little delay here
                beep_on();
                delay1ms(BEEP_SCAN_DONE_MS);
                burst_delay -= BEEP_SCAN_DONE_MS;
            }
        }
    }

    // hold trigger to burst scan
    if (key_pressed_query(KEY_TRIGGER)) {
        if (burst_delay > 0)
            delay1ms(burst_delay);

        next = FSM_STATE_SCAN;
        goto out; // jump out
    }

    if (key_pressed_query(KEY_PLUS)) {
        scan_log_idx_increase(&g_rt->read_idx[scan_mode]);
        delay1ms(150);
        goto out; // jump out
    }

    if (key_pressed_query(KEY_MINUS)) {
        scan_log_idx_decrease(&g_rt->read_idx[scan_mode]);
        delay1ms(150);
        goto out; // jump out
    }

out:
    beep_off();
    g_rt->scan_done = 0;
    g_rt->scan_mode_last = scan_mode_runtime_update();

    // user switched scan mode
    if (g_rt->scan_mode_last != scan_mode)
        g_rt->scan_show = 0;

    blink_cnt++;

    return next;
}

void state_main_exit(fsm_node_t *node, fsm_event_t event)
{
    timer3_stop();
}

fsm_state_t state_main_release_minus(fsm_node_t *node, fsm_event_t event, void *data)
{
    scan_log_idx_decrease(&g_rt->read_idx[g_rt->scan_mode]);
    return node->state;
}

fsm_state_t state_main_release_plus(fsm_node_t *node, fsm_event_t event, void *data)
{
    scan_log_idx_increase(&g_rt->read_idx[g_rt->scan_mode]);
    return node->state;
}

fsm_state_t state_main_scan_mode_switch(fsm_node_t *node, fsm_event_t event, void *data)
{
    // to display history
    uint8_t scan_mode = scan_mode_runtime_update();

    g_rt->scan_show = 0;
    g_rt->read_idx[scan_mode] = g_scan_log[scan_mode].last_write;

    return node->state;
}

static void fn_hold_timer(void *data)
{
    // double check
    if (key_released_query(KEY_FN))
        return;

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
