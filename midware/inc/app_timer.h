#ifndef __APP_TIMER_H__
#define __APP_TIMER_H__

#include "base_types.h"

#define HZ              1000

// clock src: PCLK 4MHz 256DIV
#define TIM3_PCLK_4M256D_1SEC		(15625)
#define TIM3_PCLK_4M256D_2SEC		(31250)
#define TIM3_PCLK_4M256D_3SEC		(46875)
#define TIM3_PCLK_4M256D_4SEC		(62500‬)
#define TIM3_PCLK_4M256D_MAX		(0xffff - 1)

uint16_t timer3_tick_calc(uint32_t ms);

void timer3_set(uint16_t tick,
                uint8_t oneshot,
                void (irq_callback)(void *),
                void *cb_data);
void timer3_start(void);
void timer3_stop(void);

void timer3_init(void);

uint32_t jffies_to_sec(void);
uint32_t jffies_to_msc(void);
uint32_t jffies_to_mic(void);

void basic_timer_init(void);

#endif /* __APP_TIMER_H__ */