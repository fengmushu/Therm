#ifndef __APP_KEY_H__
#define __APP_KEY_H__

#include <stdint.h>

#include "utils.h"

// NOTE: do not change the order
typedef enum gpio_key {
    KEY_MINUS = 0,
    KEY_PLUS,
    KEY_FN,
    KEY_TRIGGER,
    KEY_SWITCH,
    NUM_GPIO_KEYS,
} gpio_key_t;

extern uint8_t bm_key_pressed;

//
// polling all key and update key bitmap
//
int key_polling_update(void);

//
// check key state by gpio level
//
int key_pressed_query(gpio_key_t key);
int key_released_query(gpio_key_t key);

//
// state change by irq handler
//
static inline int is_key_pressed(gpio_key_t key)
{
    return test_bit_u8(&bm_key_pressed, key);
}

static inline int is_key_released(gpio_key_t key)
{
    return !is_key_pressed(key);
}

#endif /* __APP_KEY_H__ */