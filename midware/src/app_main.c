#include <stdint.h>

#include "fsm.h"
#include "app.h"
#include "app_lcd.h"
#include "app_i2c.h"
#include "app_key.h"
#include "app_data.h"
#include "app_timer.h"
#include "app_main.h"

#define BLINK_PERIOD_CNT                (1 << 1)

static uint8_t blink_cnt;

fsm_state_t state_main_enter(fsm_node_t *node, fsm_event_t event)
{
    uint8_t scan_mode = scan_mode_runtime_update();

    switch (event) {
    case FSM_EVENT_RELEASE_FN: // REMOVE ME
    case FSM_EVENT_IRQ_ADC: // enter from scan done
        g_rt->scan_done = 1; // to keep big number showing last scan result
        g_rt->scan_burst = 1;
        g_rt->scan_result = 295 + (blink_cnt & 127);         // TODO: get scan result from somewhere
        g_rt->scan_mode_last = scan_mode; // make sure next lcd display is matched to log

        DBG_PRINT("fake temp: %d\r\n", g_rt->scan_result);

        if (is_temp_in_range(&g_cfg->temp_thres[scan_mode], g_rt->scan_result)) {
            scan_log_write(&g_scan_log[scan_mode], g_rt->scan_result);
            g_rt->read_idx[scan_mode] = g_scan_log[scan_mode].last_write;
        }

        break;

    default: // display last write result
        g_rt->scan_done = 0;
        g_rt->scan_burst = 0;
        g_rt->read_idx[scan_mode] = g_scan_log[scan_mode].last_write;
        AppLcdClearAll();
        break;
    }

    return node->state;
}

fsm_state_t state_main_proc(fsm_node_t *node)
{
    fsm_state_t next = node->state;
    uint8_t scan_done = g_rt->scan_done;
    uint8_t scan_mode = g_rt->scan_mode_last;
    uint8_t read_idx = g_rt->read_idx[scan_mode];
    int16_t big_number;
    int16_t log_number;

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

    // read_idx should have synced to write_idx in enter() if scan_done
    log_number = scan_log_read(&g_scan_log[scan_mode], read_idx);

    if (g_cfg->temp_unit == TUNIT_F)
        log_number = lcd_show_C2F(log_number);

    AppLcdSetLogTemp(log_number, lcd_show_idx(read_idx));

    // if last state was SCAN and done
    if (scan_done) {
        big_number = g_rt->scan_result;

        if (big_number < g_cfg->temp_thres[scan_mode].low) {
            AppLcdSetString(Str_LO);
            goto lcd_update;
        } else if (big_number > g_cfg->temp_thres[scan_mode].high) {
            AppLcdSetString(Str_HI);
            goto lcd_update;
        } else {
            if (g_cfg->temp_unit == TUNIT_F)
                big_number = lcd_show_C2F(big_number);
        }
    } else {
        // user is viewing log update big number, too
        big_number = log_number;
    }

    AppLcdSetRawNumber(big_number, TRUE, 2);

lcd_update:
    AppLcdDisplayUpdate(20);
    blink_cnt++;

    // scan burst mode
    if (key_pressed_query(KEY_TRIGGER) && g_rt->scan_burst) {
        next = FSM_STATE_SCAN;
        delay1ms(1500);
    }

    if (key_pressed_query(KEY_PLUS)) {
        scan_log_idx_increase(&g_rt->read_idx[scan_mode]);
        delay1ms(150);
    }

    if (key_pressed_query(KEY_MINUS)) {
        scan_log_idx_decrease(&g_rt->read_idx[scan_mode]);
        delay1ms(150);
    }

    g_rt->scan_burst = 0;
    g_rt->scan_mode_last = scan_mode_runtime_update();

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

    g_rt->scan_done = 0;
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
