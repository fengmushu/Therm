#include <stdint.h>


#include "utils.h"
#include "fsm.h"
#include "app.h"
#include "app_data.h"
#include "app_key.h"
#include "app_lcd.h"
#include "app_fn.h"

static int8_t fn_idx;
static int8_t last_fn = -1;

enum {
	FN_TEMP_SEL = 0,
	FN_BODY_ALARM,
	FN_BODY_CAL,
	FN_BEEP_ON,
	NUM_FN,
};

struct fn_menu {
	void        (*lcd_show)(int8_t idx);
	void        (*btn_minus)(int8_t idx);
	void        (*btn_plus)(int8_t idx);
	char        big_str[4];
	char        log_str[4];
};

static struct fn_menu fn_menus[];

static void fn_temp_sel_show(int8_t idx)
{
	lcd_string_show(LCD_BIGNUM, LCD_ALIGN_RIGHT,
					fn_menus[idx].big_str, strlen(fn_menus[idx].big_str));
	lcd_string_show(LCD_LOGNUM, LCD_ALIGN_RIGHT,
					fn_menus[idx].log_str, strlen(fn_menus[idx].log_str));
	lcd_sym_temp_unit_apply(g_cfg->temp_unit);
}

static void fn_temp_sel_btn_minus(int8_t idx)
{
	g_cfg->temp_unit = TUNIT_C;
}

static void fn_temp_sel_btn_plus(int8_t idx)
{
	g_cfg->temp_unit = TUNIT_F;
}

static void fn_body_alarm_show(int8_t idx)
{
	lcd_number_show(LCD_BIGNUM, LCD_ALIGN_RIGHT,
					C2F_by_setting(g_cfg->body_alarm_C), 2, LCD_SHOW_DOT);
	lcd_string_show(LCD_LOGNUM, LCD_ALIGN_RIGHT,
					fn_menus[idx].log_str, strlen(fn_menus[idx].log_str));
	lcd_sym_temp_unit_apply(g_cfg->temp_unit);
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

static void fn_body_cal_show(int8_t idx)
{
	lcd_number_show(LCD_BIGNUM, LCD_ALIGN_RIGHT,
					g_cfg->body_cal_tweak, 2, LCD_SHOW_DOT);
	lcd_string_show(LCD_LOGNUM, LCD_ALIGN_RIGHT,
					fn_menus[idx].log_str, strlen(fn_menus[idx].log_str));
}

static void fn_body_cal_minus(int8_t idx)
{
	int16_t val = g_cfg->body_cal_tweak - 1;

	if (val >= BODY_CAL_TWEAK_MIN && val <= BODY_CAL_TWEAK_MAX)
		g_cfg->body_cal_tweak = val;
}

static void fn_body_cal_plus(int8_t idx)
{
	int16_t val = g_cfg->body_cal_tweak + 1;

	if (val >= BODY_CAL_TWEAK_MIN && val <= BODY_CAL_TWEAK_MAX)
		g_cfg->body_cal_tweak = val;
}

static void fn_beep_show(int8_t idx)
{
	lcd_string_show(LCD_BIGNUM, LCD_ALIGN_RIGHT,
					fn_menus[idx].big_str, strlen(fn_menus[idx].big_str));
	lcd_string_show(LCD_LOGNUM, LCD_ALIGN_RIGHT,
					fn_menus[idx].log_str, strlen(fn_menus[idx].log_str));
	lcd_sym_set_apply(LCD_SYM_BUZZER, g_cfg->beep_on);
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
	[FN_TEMP_SEL]   = {
		.lcd_show   = fn_temp_sel_show,
		.btn_minus  = fn_temp_sel_btn_minus,
		.btn_plus   = fn_temp_sel_btn_plus,
		.big_str    = "F1",
		.log_str    = "unit",
	},
	[FN_BODY_ALARM] = {
		.lcd_show   = fn_body_alarm_show,
		.btn_minus  = fn_body_alarm_btn_minus,
		.btn_plus   = fn_body_alarm_btn_plus,
		.big_str    = "F2",
		.log_str    = "thrs",
	},
	[FN_BODY_CAL]   = {
		.lcd_show   = fn_body_cal_show,
		.btn_minus  = fn_body_cal_minus,
		.btn_plus   = fn_body_cal_plus,
		.big_str    = "F3",
		.log_str    = "CAL",
	},
	[FN_BEEP_ON]    = {
		.lcd_show   = fn_beep_show,
		.btn_minus  = fn_beep_minus,
		.btn_plus   = fn_beep_plus,
		.big_str    = "F4",
		.log_str    = "BEEP",
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
		delay1ms(500); // to prevent switch options too fast
		goto out;
	}

	if (key_pressed_query(KEY_PLUS)) {
		app_fn_btn_plus();
		delay1ms(250); // to reduce burst speed
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
