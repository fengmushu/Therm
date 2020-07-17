#include <stdint.h>

#include "app.h"
#include "fsm.h"
#include "utils.h"
#include "app_gpio.h"
#include "app_key.h"
#include "app_rtc.h"

#define GPIO_LO                     (FALSE)
#define GPIO_HI                     (TRUE)

#define GPIO_KEY_PRESS(port, pin)   (Gpio_GetInputIO((port), (pin)) == GPIO_LO)
#define GPIO_KEY_RELEASE(port, pin) (Gpio_GetInputIO((port), (pin)) == GPIO_HI)

// WARNING: limited key count, enlarge the size of bitmap if over 8 keys
uint8_t bm_key_pressed;

typedef struct key_map {
        uint8_t gpio_port;
        uint8_t gpio_pin;
        uint8_t event_press;    // currently defined as GPIO LO
        uint8_t event_release;  // currently defined as GPIO HI
} key_map_t;

static key_map_t keys_map[] = {
        [KEY_MINUS]   = { M_KEY_LEFT_PORT,   M_KEY_LEFT_PIN,   FSM_EVENT_PRESS_MINUS,   FSM_EVENT_RELEASE_MINUS   },
        [KEY_PLUS]    = { M_KEY_MID_PORT,    M_KEY_MID_PIN,    FSM_EVENT_PRESS_PLUS,    FSM_EVENT_RELEASE_PLUS    },
        [KEY_FN]      = { M_KEY_RIGHT_PORT,  M_KEY_RIGHT_PIN,  FSM_EVENT_PRESS_FN,      FSM_EVENT_RELEASE_FN      },
        [KEY_TRIGGER] = { M_KEY_TRIG_PORT,   M_KEY_TRIG_PIN,   FSM_EVENT_PRESS_TRIGGER, FSM_EVENT_RELEASE_TRIGGER },
        [KEY_SWITCH]  = { M_KEY_SWITCH_PORT, M_KEY_SWITCH_PIN, FSM_EVENT_SWITCH_BODY,   FSM_EVENT_SWITCH_SURFACE  },
};

static uint8_t fsm_state_key_pressed[] = {
        [0 ... NUM_GPIO_KEYS ] = __FSM_STATE_NONE,
};

static __always_inline void __key_pressed(gpio_key_t key)
{
        set_bit_u8(&bm_key_pressed, key);
}

static __always_inline void __key_released(gpio_key_t key)
{
        clear_bit_u8(&bm_key_pressed, key);
}

int key_pressed_query(gpio_key_t key)
{
        return GPIO_KEY_PRESS(keys_map[key].gpio_port, keys_map[key].gpio_pin);
}

int key_released_query(gpio_key_t key)
{
        return GPIO_KEY_RELEASE(keys_map[key].gpio_port, keys_map[key].gpio_pin);
}

void key_poll_once(void)
{
        __disable_irq();

        for (int i = 0; i < NUM_GPIO_KEYS; i++) {
                if (key_pressed_query(i))
                        __key_pressed(i);
                else
                        __key_released(i);
        }

        __enable_irq();
}

static __always_inline void key_gpio_irq_handle(int i)
{
        fsm_event_t event;
        uint8_t *last_pressed = &fsm_state_key_pressed[i];

        if (Gpio_GetIrqStatus(keys_map[i].gpio_port, keys_map[i].gpio_pin) == FALSE)
                return;

        Gpio_ClearIrq(keys_map[i].gpio_port, keys_map[i].gpio_pin);

        if (GPIO_KEY_PRESS(keys_map[i].gpio_port, keys_map[i].gpio_pin)) {
                event = keys_map[i].event_press;
                __key_pressed(i);

                *last_pressed = g_fsm.curr->state;
        } else {
                event = keys_map[i].event_release;
                __key_released(i);

                // BUG:
                // if return back to same state,
                // the key just released, release event still will input
                // need to clear to prevent input which was intended for previous state

                // avoid posting releasing event into wrong state
                if (*last_pressed != g_fsm.curr->state &&
                        *last_pressed != __FSM_STATE_NONE) {
                        // clear to allow new release input
                        *last_pressed = __FSM_STATE_NONE;
                        return;
                }
        }

        fsm_event_post(&g_fsm, FSM_EVENT_RING_PRIO_LO, event);
}

void PortC_IRQHandler(void)
{
        AppRtcFeed();

        for (int i = KEY_MINUS; i <= KEY_FN; i++)
                key_gpio_irq_handle(i);
}

void PortD_IRQHandler(void)
{
        AppRtcFeed();

        for (int i = KEY_TRIGGER; i <= KEY_SWITCH; i++)
                key_gpio_irq_handle(i);
}