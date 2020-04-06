#ifndef __RING_BUF_H__
#define __RING_BUF_H__

#include <stdint.h>

// use power of 2 to make (&) mod work
#define RING_BUF_CNT		(1 << 4)

#define RING_NEXT_IDX(x)	(((x) + 1) & (RING_BUF_CNT - 1))

typedef struct fsm_ring_data {
	uint8_t event;
	uint8_t state;
} event_ring_data_t;

typedef struct event_ring {
	event_ring_data_t buf[RING_BUF_CNT];
	uint8_t 	  write; // aka. head: next idx to write
	uint8_t 	  read;	 // aka. tail: next idx to read
	uint8_t 	  full;	 // trick to determine whether full when *read == *write
			         // using @full is not thread-safe
} event_ring_t;

int event_ring_reset(event_ring_t *ring);
int event_ring_is_empty(event_ring_t *ring);
int event_ring_is_full(event_ring_t *ring);
int event_ring_size(event_ring_t *ring);
int event_ring_used(event_ring_t *ring);
void event_ring_put_override(event_ring_t *ring, event_ring_data_t *data);
int event_ring_put(event_ring_t *ring, event_ring_data_t *data);
int event_ring_get(event_ring_t *ring, event_ring_data_t *data);

#endif /* __RING_BUF_H__ */