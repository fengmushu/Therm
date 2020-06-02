#include <stdint.h>

#include "app.h"
#include "app_key.h"
#include "app_lcd.h"
#include "app_data.h"
#include "app_factory.h"

uint8_t    factory_mode;

static int is_factory_test(void)
{
        return 0;
}

static void factory_test_init(void)
{
        AppLcdEnable();
}

static void factory_lcd_test(void)
{
        for (int i = LedRed; i <= LedOrange; i++) {
                AppLcdDisplayAll();
                delay1ms(500);
        }
}

static void factory_key_test(void)
{
        uint8_t led_color = 1;

        while (1) {
                AppLcdDisplayAll();
                delay1ms(30);

                if (key_pressed_query(KEY_TRIGGER)) {
                        beep_on();
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

        factory_key_test();
}
