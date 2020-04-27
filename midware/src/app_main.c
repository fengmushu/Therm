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

#define BEEP_SCAN_DONE_MS               (100)

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

        if (is_temp_valid(&g_temp_thres[scan_mode], last_result)) {
            // last_write++ in scan_log_write_safe()
            scan_log_write_safe(scan_log, last_result);
            g_rt->read_idx[scan_mode] = scan_log->last_write;

#ifdef FACTORY_MODE_UV_DEBUG
            scan_log_write_idx(&log_uv[scan_mode], scan_log->last_write, last_uv);
            scan_log_write_idx(&log_ntc[scan_mode], scan_log->last_write, last_ntc);
#endif
        }

        break;

    default: // display last write result
        g_rt->scan_show = 0;
        g_rt->scan_done = 0;
        g_rt->read_idx[scan_mode] = scan_log->last_write;

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
    uint8_t scan_mode = g_cfg->scan_mode;
    uint8_t read_idx = g_rt->read_idx[scan_mode];

    // by testing, say 5 at least to stabilize lcd
    const int16_t lcd_budget_ms = 20;
    const int16_t key_budget_ms = 150;
    int16_t delay_budget = lcd_budget_ms;

    int16_t big_number;
    int16_t log_number;

    // reset color for all not defined patterns
    AppLedEnable(LedGreen);

    AppLcdSetBuzzer(g_cfg->beep_on);
    AppLcdSetTempMode(g_cfg->temp_unit, TRUE);
    AppLcdSetCheckMode(g_cfg->scan_mode, TRUE);
    AppLcdSetBattery(TRUE, g_rt->battery_lvl);

    AppLcdSetSymbol(SAD_SYM, FALSE);
    AppLcdSetSymbol(SMILE_SYM, FALSE);

    // blinking
    if (g_rt->battery_lvl == BAT_LVL_CRIT) {
        if (blink_is_on_duty(BLINK_DUTY_50, 4))
            AppLcdSetBattery(FALSE, g_rt->battery_lvl);
    }

    // read_idx should have synced to write_idx in enter() if comes from scan
    log_number = scan_log_read(&g_scan_log[scan_mode], read_idx);

    AppLcdSetLogIndex(TRUE, lcd_show_idx(read_idx));

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

#ifdef FACTORY_MODE_UV_DEBUG
    if (dbg_show_uv) {
        uint16_t ntc_number = scan_log_read(&log_ntc[scan_mode], read_idx);
        log_number = scan_log_read(&log_uv[scan_mode], read_idx);

        AppLcdSetLogIndex(FALSE, ntc_number);

        if (blink_is_on_duty(BLINK_DUTY_50, 4)) {
            AppLcdSetRawNumber(log_number, FALSE, 4);
            goto lcd_update;
        }
    }
#endif

    //
    // surface mode is always green, and LED is set green already
    //
    if (scan_mode == SCAN_BODY) {
        if (big_number > BODY_TEMP_UNDERFLOW_C) {
            AppLcdSetSymbol(SMILE_SYM, TRUE);
            AppLcdSetSymbol(SAD_SYM, FALSE);
        }

        if (big_number >= BODY_FEVER_LOW) {
            AppLedEnable(LedOrange);
            AppLcdSetSymbol(SMILE_SYM, FALSE);
            AppLcdSetSymbol(SAD_SYM, TRUE);
        }

        // currently, body_alarm_C can be smaller than BODY_FEVER_LOW
        if (big_number >= g_cfg->body_alarm_C) {
            AppLedEnable(LedRed);
            AppLcdSetSymbol(SMILE_SYM, FALSE);
            AppLcdSetSymbol(SAD_SYM, TRUE);
        }
    }

    AppLcdSetRawNumber(C2F_by_setting(big_number), TRUE, 2);

lcd_update:
    AppLcdDisplayUpdate(0);

    // scan_done will be oneshot after scan done
    if (g_rt->scan_done) {
        if (g_cfg->beep_on) {
            if (scan_mode == SCAN_BODY && big_number >= g_cfg->body_alarm_C) {
                delay_budget -= body_beep_alarm();
            } else {
                // user may wanna release trigger after hearing beep
                // so put a little delay here
                beep_once(BEEP_SCAN_DONE_MS);
                delay_budget -= BEEP_SCAN_DONE_MS;
            }
        }
    }

    // hold trigger to burst scan
    if (key_pressed_query(KEY_TRIGGER)) {
        next = FSM_STATE_SCAN;
        goto out; // jump out
    }

    if (key_pressed_query(KEY_LOG)) {
        // FIXME: we need to check whether log is once full
        scan_log_idx_decrease(&g_rt->read_idx[scan_mode]);
        delay_budget += key_budget_ms;
        g_rt->scan_show = 0;

        goto delay;
    }

delay:
    // share delay for key, lcd, beep
    if (delay_budget > 0)
        delay1ms(delay_budget);

out:
    g_rt->scan_done = 0;
    blink_inc();

    return next;
}

void state_main_exit(fsm_node_t *node, fsm_event_t event)
{
    UNUSED_PARAM(node);
    UNUSED_PARAM(event);
}
