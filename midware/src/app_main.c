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
    scan_log_t *scan_log = &g_scan_log[scan_mode];

    switch (event) {
    case FSM_EVENT_SCAN_DONE:
        AppRtcFeed();

        g_rt->scan_show = 1; // to keep big number showing last scan result
        g_rt->scan_done = 1;
        g_rt->scan_mode_last = scan_mode; // make sure next lcd display is matched to log

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
        g_rt->scan_mode_last = scan_mode;
        g_rt->read_idx[scan_mode] = scan_log->last_write;

        lcd_display_clear();

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
    uint8_t led_color = LedGreen;

    const int16_t key_budget_ms = 150;
    int16_t delay_budget = 0;

    int16_t big_number;
    int16_t log_number;

    lcd_sym_set(LCD_SYM_LOCK, 0);
    lcd_sym_set(LCD_SYM_BUZZER, g_cfg->beep_on);
    lcd_sym_temp_unit_set(g_cfg->temp_unit);
    lcd_sym_scan_mode_set(g_rt->scan_mode);

    // blinking
    if (g_rt->battery_low) {
        uint8_t duty;

        duty = blink_cnt & GENMASK(5, 4);
        duty >>= 4;

        lcd_sym_set(LCD_SYM_BAT, 0);

        if (duty == 0x00 || duty == 0x01) // duty 50%
            lcd_sym_set(LCD_SYM_BAT, 1);
    }

    // read_idx should have synced to write_idx in enter() if comes from scan
    log_number = scan_log_read(&g_scan_log[scan_mode], read_idx);

    lcd_sym_set(LCD_SYM_TEXT_LOG, 1);
    lcd_float1_show(LCD_LOGNUM, C2F_by_setting(log_number));
    lcd_number_show(LCD_IDXNUM, LCD_ALIGN_LEFT, lcd_show_idx(read_idx), 2, LCD_NO_DOT);

    // user is viewing log, show big number as log number
    big_number = log_number;

#ifdef FACTORY_MODE_UV_DEBUG
    if (log_show_uv) {
        uint16_t ntc_number = scan_log_read(&log_ntc[scan_mode], read_idx);
        log_number = scan_log_read(&log_uv[scan_mode], read_idx);
        lcd_sym_set(LCD_SYM_TEXT_LOG, 0);
        lcd_number_show(LCD_LOGNUM, LCD_ALIGN_LEFT, log_number, 0, LCD_NO_DOT);
        lcd_number_show(LCD_IDXNUM, LCD_ALIGN_LEFT, ntc_number, 0, LCD_NO_DOT);
    }
#endif

    if (scan_show) {
        big_number = g_rt->scan_result[scan_mode];

        if (big_number < g_temp_thres[scan_mode].underflow) {
            led_color = LedOrange;
            lcd_string_show(LCD_BIGNUM, LCD_ALIGN_RIGHT, "Lo", 2);
            goto lcd_update; // jump out
        } else if (big_number > g_temp_thres[scan_mode].overflow) {
            led_color = LedRed;
            lcd_string_show(LCD_BIGNUM, LCD_ALIGN_RIGHT, "Hi", 2);
            goto lcd_update; // jump out
        }
    }

    //
    // surface mode is always green, and LED is set green already
    //
    if (scan_mode == SCAN_BODY) {
        // currently, body_alarm_C can be smaller than BODY_FEVER_LOW
        if (big_number >= g_cfg->body_alarm_C) {
            led_color = LedRed;
        } else if (big_number >= BODY_FEVER_LOW) {
            led_color = LedOrange;
        }
    }

    lcd_float1_show(LCD_BIGNUM, C2F_by_setting(big_number));

lcd_update:
    AppLedEnable(led_color);

    // lcd symbol batch update here
    // if lcd_sym_set_apply() used above, may cause flickers
    lcd_sym_list_apply();

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

    if (key_pressed_query(KEY_PLUS)) {
        scan_log_idx_increase(&g_rt->read_idx[scan_mode]);
        delay_budget += key_budget_ms;
        goto delay;
    }

    if (key_pressed_query(KEY_MINUS)) {
        scan_log_idx_decrease(&g_rt->read_idx[scan_mode]);
        delay_budget += key_budget_ms;
        goto delay;
    }

delay:
    // share delay for key, lcd, beep
    if (delay_budget > 0)
        delay1ms(delay_budget);

out:
    g_rt->scan_done = 0;
    g_rt->scan_mode_last = scan_mode_runtime_update();

    // user switched scan mode
    if (g_rt->scan_mode_last != scan_mode) {
        g_rt->scan_show = 0;
        scan_mode = g_rt->scan_mode_last;
        g_rt->read_idx[scan_mode] = g_scan_log[scan_mode].last_write;
    }

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
    timer3_set(TIM3_PCLK_4M256D_1SEC, 1, fn_hold_timer, NULL);
    timer3_start();

    return node->state;
}

fsm_state_t state_main_release_fn(fsm_node_t *node, fsm_event_t event, void *data)
{
    timer3_stop();

    return node->state;
}
