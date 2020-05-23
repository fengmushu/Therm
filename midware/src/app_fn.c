#include <stdint.h>

#include "utils.h"
#include "fsm.h"
#include "app.h"
#include "app_data.h"
#include "app_key.h"
#include "app_lcd.h"
#include "app_fn.h"
#include "app_timer.h"

static int8_t fn_idx;
static int8_t last_fn = -1;

enum {
    FN_SCAN_MODE_SEL = 0,
    FN_TUNIT_SEL,
    FN_BODY_ALARM,
    FN_BODY_CAL,
    FN_BEEP_ON,
    NUM_FN,
};

struct fn_menu {
    void        (*lcd_show)(int8_t idx);
    void        (*opt_next)(int8_t idx);
    char        big_str[4];
    uint8_t     hidden;
};

static struct fn_menu fn_menus[];

static __always_inline int opt_cycle_inc(int val, int max)
{
     return (val + 1) % max;
}

static __always_inline int opt_cycle_inc_in_range(int val, int min, int max)
{
    val++;

    if (val > max)
        return min;

    return val;
}

static void fn_scmode_sel_show(int8_t idx)
{
    uint8_t scan_mode = g_cfg->scan_mode;

    lcd_string_show(LCD_BIGNUM,
                    LCD_ALIGN_RIGHT,
                    fn_menus[idx].big_str,
                    strlen(fn_menus[idx].big_str));

    if (blink_is_on_duty(BLINK_DUTY_50, 8))
        scan_mode = INVALID_SCAN_MODE;

    lcd_sym_scan_mode_apply(scan_mode);
}

static void fn_scmode_opt_next(int8_t idx)
{
    g_cfg->scan_mode = opt_cycle_inc(g_cfg->scan_mode, NUM_SCAN_MODES);
}

static void fn_tunit_sel_show(int8_t idx)
{
    uint8_t tunit = g_cfg->temp_unit;

    lcd_string_show(LCD_BIGNUM,
                    LCD_ALIGN_RIGHT,
                    fn_menus[idx].big_str,
                    strlen(fn_menus[idx].big_str));

    if (blink_is_on_duty(BLINK_DUTY_50, 8))
        tunit = INVALID_TUNIT;

    lcd_sym_temp_unit_apply(tunit);
}

static void fn_tunit_opt_next(int8_t idx)
{
    g_cfg->temp_unit = opt_cycle_inc(g_cfg->temp_unit, NUM_TEMP_UNITS);
}

static void fn_body_alarm_show(int8_t idx)
{
    lcd_temperature_show(LCD_BIGNUM, C2F_by_setting(g_cfg->body_alarm_C));
    lcd_sym_temp_unit_apply(g_cfg->temp_unit);
}

static void fn_body_alarm_opt_next(int8_t idx)
{
    g_cfg->body_alarm_C = opt_cycle_inc_in_range(g_cfg->body_alarm_C,
                                                 BODY_ALARM_THRESH_MIN,
                                                 BODY_ALARM_THRESH_MAX);
}

static void fn_body_cal_show(int8_t idx)
{
    lcd_number_show(LCD_BIGNUM, LCD_ALIGN_RIGHT,
                    g_cfg->body_cal_tweak, 2, LCD_SHOW_DOT);
}

static void fn_body_cal_opt_next(int8_t idx)
{
    g_cfg->body_cal_tweak = opt_cycle_inc_in_range(g_cfg->body_cal_tweak,
                                                   BODY_CAL_TWEAK_MIN,
                                                   BODY_CAL_TWEAK_MAX);
}

static void fn_beep_on_show(int8_t idx)
{
    lcd_string_show(LCD_BIGNUM, LCD_ALIGN_LEFT,
                    fn_menus[idx].big_str, strlen(fn_menus[idx].big_str));
}

static void fn_beep_on_opt_next(int8_t idx)
{
    g_cfg->beep_on = opt_cycle_inc(g_cfg->beep_on, NUM_BEEP_MODES);

    if (g_cfg->beep_on == BEEP_ON)
        beep_once(200);
}

fsm_state_t fsm_state_beep_cycle(fsm_node_t *node,
                                 fsm_event_t event,
                                 void *data)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(event);

    fn_beep_on_opt_next(FN_BEEP_ON);

    return node->state;
}

static struct fn_menu fn_menus[] = {
    [FN_SCAN_MODE_SEL] = {
        .lcd_show      = fn_scmode_sel_show,
        .opt_next      = fn_scmode_opt_next,
        .big_str       = "F1",
        .hidden        = 0,
    },
    [FN_TUNIT_SEL]     = {
        .lcd_show      = fn_tunit_sel_show,
        .opt_next      = fn_tunit_opt_next,
        .big_str       = "F2",
        .hidden        = 0,
    },
    [FN_BODY_ALARM]    = {
        .lcd_show      = fn_body_alarm_show,
        .opt_next      = fn_body_alarm_opt_next,
        .big_str       = "F3",
        .hidden        = 1,
    },
    [FN_BODY_CAL]      = {
        .lcd_show      = fn_body_cal_show,
        .opt_next      = fn_body_cal_opt_next,
        .big_str       = "F4",
        .hidden        = 1,
    },
    [FN_BEEP_ON]      = {
        .lcd_show      = fn_beep_on_show,
        .opt_next      = fn_beep_on_opt_next,
        .big_str       = "BEEP",
        .hidden        = 1,
    },
};

static void app_sub_fn_enter(int8_t idx)
{
    lcd_string_clean_show(LCD_BIGNUM, LCD_ALIGN_RIGHT, 300,
                          fn_menus[idx].big_str, strlen(fn_menus[idx].big_str));
    lcd_display_clear();
}

void app_fn_next(void)
{
    if (fn_idx >= 0 && fn_idx < NUM_FN)
        fn_idx++;
}

void app_fn_enter(void)
{
    fn_idx = 0;
    last_fn = -1;

    AppLedEnable(LedOrange);
}

void app_fn_exit(void)
{
    char str[] = "SAVE";
    fn_idx = -1;

    app_save_i2c_config_only(g_save);

    lcd_string_clean_show(LCD_BIGNUM, LCD_ALIGN_RIGHT, 500, str, sizeof(str));
    lcd_display_clear();
}

void app_fn_next_opt(void)
{
    int8_t idx = fn_idx;

    if (idx >= 0 && idx < NUM_FN)
        fn_menus[idx].opt_next(idx);
}

// loop by fsm_process()
int app_fn_proc(void)
{
    uint8_t i = fn_idx;

    if (i < 0 || i >= NUM_FN) {
        lcd_display_clear();
        return APP_FN_DONE;
    }

    if (fn_menus[i].hidden) {
        app_fn_next();
        goto out;
    }

    // lcd cleared here
    if (last_fn != i) {
        last_fn = i;
        app_sub_fn_enter(i);
    }

    if (fn_menus[i].lcd_show)
        fn_menus[i].lcd_show(i);

    if (key_pressed_query(KEY_FN)) {
        app_fn_next();
        delay1ms(500);
        goto out;
    }

    if (key_pressed_query(KEY_LOG)) {
        app_fn_next_opt();
        delay1ms(250);
        goto out;
    }

out:
    blink_inc();
    return APP_FN_OK;
}
