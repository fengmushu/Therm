#ifndef __APP_TIMER_H__
#define __APP_TIMER_H__

#include <stdint.h>
#include "utils.h"

// clock src: PCLK 4MHz 256DIV
#define TIM3_PCLK_4M256D_1SEC		(15625)
#define TIM3_PCLK_4M256D_2SEC		(31250)
#define TIM3_PCLK_4M256D_3SEC		(46875)
#define TIM3_PCLK_4M256D_4SEC		(62500‬)
#define TIM3_PCLK_4M256D_MAX		(0xffff - 1)

enum {
	BLINK_DUTY_0 = 0,
	BLINK_DUTY_25,
	BLINK_DUTY_50,
	BLINK_DUTY_75,
	BLINK_DUTY_100,
};

extern uint32_t blink_cnt;

uint16_t timer3_tick_calc(uint32_t ms);

void timer3_set(uint16_t tick,
                uint8_t oneshot,
                void (irq_callback)(void *),
                void *cb_data);
void timer3_start(void);
void timer3_stop(void);

void timer3_init(void);

static __always_inline void blink_inc(void)
{
	blink_cnt++;
}

static __always_inline int blink_is_on_duty(int duty, uint8_t intv)
{
	unsigned d;

	if (intv >= (sizeof(blink_cnt) * BITS_PER_BYTE - 1))
		return 0;

	// larger interval, more blink_cnt need to count
	d = blink_cnt & GENMASK((intv + 1), intv);
	d >>= intv;

	switch (duty) {
	case BLINK_DUTY_0:
		return 0;

	case BLINK_DUTY_25:
		if (d == 0x00)
			return 1;

		return 0;

	case BLINK_DUTY_50:
		if (d == 0x00 || d == 0x01)
			return 1;

		return 0;

	case BLINK_DUTY_75:
		if (d != 0x00)
			return 1;

		return 0;

	case BLINK_DUTY_100:
	default:
		return 1;
	}
}

#endif /* __APP_TIMER_H__ */