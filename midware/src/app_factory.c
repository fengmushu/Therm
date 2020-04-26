#include <stdint.h>

#include "app.h"
#include "app_key.h"
#include "app_lcd.h"
#include "app_data.h"
#include "app_factory.h"

uint8_t    factory_mode;

#ifdef FACTORY_MODE_UV_DEBUG
uint8_t    log_show_uv;
uint32_t   last_uv, last_ntc;
scan_log_t log_uv[NUM_SCAN_MODES];
scan_log_t log_ntc[NUM_SCAN_MODES];
#endif

static int is_factory_test(void)
{
        if (key_pressed_query(KEY_BEEP))
                return 1;

        return 0;
}

#ifdef FACTORY_MODE_UV_DEBUG
static int is_factory_debug(void)
{
        if (key_pressed_query(KEY_LOG))
                return 1;

        return 0;
}
#endif

static void factory_test_init(void)
{
        AppLcdEnable();
        AppLedEnable(LedOrange);
}

static void factory_lcd_test(void)
{
        for (int i = LedRed; i <= LedOrange; i++) {
                AppLedEnable(i);

                AppLcdDisplayAll();
                delay1ms(500);
                AppLcdDisplayClear();
                delay1ms(500);
        }
}

static void factory_key_test(void)
{
        uint8_t led_color = 1;

        while (1) {
                AppLcdDisplayAll();
                delay1ms(30);

                if (key_pressed_query(KEY_BEEP)) {
                        AppLedEnable(LedOrange);
                        beep_on();
                        AppLcdDisplayClear();
                        key_wait_for_release(KEY_BEEP);
                        beep_off();
                        continue;
                }

                if (key_pressed_query(KEY_LOG)) {
                        AppLedEnable(LedOrange);
                        beep_on();
                        AppLcdDisplayClear();
                        key_wait_for_release(KEY_LOG);
                        beep_off();
                        continue;
                }

                if (key_pressed_query(KEY_FN)) {
                        AppLedEnable(LedOrange);
                        beep_on();
                        AppLcdDisplayClear();
                        key_wait_for_release(KEY_FN);
                        beep_off();
                        continue;
                }

                if (key_pressed_query(KEY_TRIGGER)) {
                        AppLedEnable(LedOrange);
                        beep_on();
                        AppLcdDisplayClear();
                        key_wait_for_release(KEY_TRIGGER);
                        beep_off();
                        continue;
                }
        }
}

void factory_test(void)
{
        if (!is_factory_test())
                return;

        factory_mode = 1;

        factory_test_init();
        factory_lcd_test();

#ifdef FACTORY_MODE_UV_DEBUG
        if (is_factory_debug()) {
                log_show_uv = 1;
                beep_once(200);
                return;
        }
#endif

        factory_key_test();
}
