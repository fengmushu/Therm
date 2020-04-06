#include <stdint.h>

#include "app.h"
#include "fsm.h"
#include "utils.h"
#include "app_gpio.h"
#include "app_key.h"

#define GPIO_LO                     (FALSE)
#define GPIO_HI                     (TRUE)

#define GPIO_KEY_PRESS(port, pin)   (Gpio_GetInputIO((port), (pin)) == GPIO_LO)
#define GPIO_KEY_RELEASE(port, pin) (Gpio_GetInputIO((port), (pin)) == GPIO_HI)

// WARNING: limited key count, enlarge the size of bitmap if over 8 keys
uint8_t bm_key_pressed;

// when enter a new state, block keys event that entered in last states
uint8_t bm_key_drop;

typedef struct key_map {
    uint8_t gpio_port;
    uint8_t gpio_pin;
    uint8_t event_press;
    uint8_t event_release;
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

static inline void key_pressed(gpio_key_t key)
{
    set_bit_u8(&bm_key_pressed, key);
}

static inline void key_released(gpio_key_t key)
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
            key_pressed(i);
        else
            key_released(i);
    }

    __enable_irq();
}

void key_drop_mark(void)
{
    key_poll_once();
    bm_key_drop = bm_key_pressed;

    // workaround: 
    // not to mark switch key
    // which might always be high or low
    clear_bit_u8(&bm_key_drop, KEY_SWITCH);
}

static inline void key_drop_clear(gpio_key_t key)
{
    clear_bit_u8(&bm_key_drop, key);
}

static inline void key_gpio_irq_handle(int i)
{
    fsm_event_t event;
    uint8_t *last_pressed = &fsm_state_key_pressed[i];

    if (Gpio_GetIrqStatus(keys_map[i].gpio_port, keys_map[i].gpio_pin) == FALSE)
        return;

    Gpio_ClearIrq(keys_map[i].gpio_port, keys_map[i].gpio_pin);

    if (GPIO_KEY_PRESS(keys_map[i].gpio_port, keys_map[i].gpio_pin)) {
        event = keys_map[i].event_press;
        key_pressed(i);

        *last_pressed = g_fsm.curr->state;
    } else {
        event = keys_map[i].event_release;
        key_released(i);

        // avoid posting releasing event into wrong state
        if (*last_pressed != g_fsm.curr->state &&
            *last_pressed != __FSM_STATE_NONE) {
            // clear to allow new release input
            *last_pressed = __FSM_STATE_NONE;
            return;
        }

        // if (test_bit_u8(&bm_key_drop, i)) {
        //     key_drop_clear(i);
        //     return;
        // }
    }

    fsm_event_post(&g_fsm, FSM_EVENT_RING_PRIO_LO, event);
}

void PortC_IRQHandler(void)
{
    delay1ms(100);

    // REVIEW: key_switch checked redundantly
    for (int i = KEY_MINUS; i <= KEY_FN; i++)
        key_gpio_irq_handle(i);

    printf("%s: key bitmap: %02x\r\n", __func__, bm_key_pressed);
}

void PortD_IRQHandler(void)
{
    delay1ms(100);

    for (int i = KEY_TRIGGER; i <= KEY_SWITCH; i++)
        key_gpio_irq_handle(i);

    printf("%s: key bitmap: %02x\r\n", __func__, bm_key_pressed);
}