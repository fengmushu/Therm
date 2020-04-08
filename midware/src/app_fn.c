#include <stdint.h>


#include "utils.h"
#include "fsm.h"
#include "app.h"
#include "app_data.h"
#include "app_key.h"
#include "app_lcd.h"
#include "app_fn.h"

#define FN_SHOW_LCD_STAY_MS             (30)
#define FN_TITLE_LCD_STAY_MS            (1000)

static int8_t fn_idx;
static int8_t last_fn = -1;

enum {
    FN_TEMP_SEL = 0,
    FN_BODY_ALARM,
    FN_CAL_OFFSET,
    FN_BEEP_ON,
    NUM_FN,
};

struct fn_menu {
    void        (*lcd_show)(int8_t idx);
    void        (*btn_minus)(int8_t idx);
    void        (*btn_plus)(int8_t idx);
    uint8_t     lcd_str;
};

static struct fn_menu fn_menus[];

static void fn_temp_sel_show(int8_t idx)
{
    AppLcdSetString(fn_menus[idx].lcd_str);
    AppLcdSetTempMode(g_cfg->temp_unit, TRUE);
    AppLcdDisplayUpdate(FN_SHOW_LCD_STAY_MS);
}

static void fn_temp_sel_btn_minus(int8_t idx)
{
    g_cfg->temp_unit = Celsius;
}

static void fn_temp_sel_btn_plus(int8_t idx)
{
    g_cfg->temp_unit = Fahrenheit;
}

static void fn_body_alarm_show(int8_t idx)
{
    int16_t t = g_cfg->body_alarm_C;

    if (g_cfg->temp_unit == TUNIT_F)
        t = lcd_show_C2F(t);

    AppLcdSetTemp(t);
    AppLcdSetTempMode(g_cfg->temp_unit, TRUE);
    AppLcdDisplayUpdate(FN_SHOW_LCD_STAY_MS);
}

static void fn_body_alarm_btn_minus(int8_t idx)
{
    uint16_t val = g_cfg->body_alarm_C - 1;

    if (val >= BODY_ALARM_THRESH_MIN && val <= BODY_ALARM_THRESH_MAX)
        g_cfg->body_alarm_C = val;
}

static void fn_body_alarm_btn_plus(int8_t idx)
{
    uint16_t val = g_cfg->body_alarm_C + 1;
    
    if (val >= BODY_ALARM_THRESH_MIN && val <= BODY_ALARM_THRESH_MAX)
        g_cfg->body_alarm_C = val;
}

static void fn_cal_offset_show(int8_t idx)
{
    AppLcdSetRawNumber(g_cfg->cal_offset, TRUE, 2);
    AppLcdDisplayUpdate(FN_SHOW_LCD_STAY_MS);
}

static void fn_cal_offset_minus(int8_t idx)
{
    int16_t val = g_cfg->cal_offset - 1;

    if (val >= CAL_OFFSET_MIN && val <= CAL_OFFSET_MAX)
        g_cfg->cal_offset = val;
}

static void fn_cal_offset_plus(int8_t idx)
{
    int16_t val = g_cfg->cal_offset + 1;
    
    if (val >= CAL_OFFSET_MIN && val <= CAL_OFFSET_MAX)
        g_cfg->cal_offset = val;
}

static void fn_beep_show(int8_t idx)
{
    AppLcdSetString(fn_menus[idx].lcd_str);
    AppLcdSetBuzzer(g_cfg->beep_on);
    AppLcdDisplayUpdate(FN_SHOW_LCD_STAY_MS);
}

static void fn_beep_minus(int8_t idx)
{
    g_cfg->beep_on = BEEP_OFF;
}

static void fn_beep_plus(int8_t idx)
{
    g_cfg->beep_on = BEEP_ON;
    beep_once(250);
}

static struct fn_menu fn_menus[] = {
    [FN_TEMP_SEL]   = { fn_temp_sel_show,   fn_temp_sel_btn_minus,   fn_temp_sel_btn_plus,   Str_F1 },
    [FN_BODY_ALARM] = { fn_body_alarm_show, fn_body_alarm_btn_minus, fn_body_alarm_btn_plus, Str_F2 },
    [FN_CAL_OFFSET] = { fn_cal_offset_show, fn_cal_offset_minus,     fn_cal_offset_plus,     Str_F3 },
    [FN_BEEP_ON]    = { fn_beep_show,       fn_beep_minus,           fn_beep_plus,           Str_F4 },
};

static void app_sub_fn_enter(int8_t idx)
{
    AppLcdClearAll();
    AppLcdSetString(fn_menus[idx].lcd_str);
    AppLcdDisplayUpdate(FN_TITLE_LCD_STAY_MS);
    AppLcdClearAll();
    AppLcdDisplayUpdate(0);
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
}

void app_fn_exit(void)
{
    fn_idx = -1;
}

void app_fn_btn_plus(void)
{
    int8_t idx = fn_idx;

    if (idx >= 0 && idx < NUM_FN)
        fn_menus[idx].btn_plus(idx);
}

void app_fn_btn_minus(void)
{
    int8_t idx = fn_idx;

    if (idx >= 0 && idx < NUM_FN)
        fn_menus[idx].btn_minus(idx);
}

// loop by fsm_process()
int app_fn_proc(void)
{
    uint8_t i = fn_idx;

    if (i < 0 || i >= NUM_FN) {
        AppLcdClearAll();
        return APP_FN_DONE;
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

    if (key_pressed_query(KEY_PLUS)) {
        app_fn_btn_plus();
        delay1ms(250);
        goto out;
    }

    if (key_pressed_query(KEY_MINUS)) {
        app_fn_btn_minus();
        delay1ms(250);
        goto out;
    }

out:
    return APP_FN_OK;
}
