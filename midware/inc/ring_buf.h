#ifndef __RING_BUF_H__
#define __RING_BUF_H__

#include <stdint.h>

// use power of 2 to make (&) mod work
#define RING_BUF_SIZE		(1 << 4)

#define RING_NEXT_IDX(x)	(((x) + 1) & (RING_BUF_SIZE - 1))

typedef struct ring_buf {
	uint8_t buf[RING_BUF_SIZE];
	uint8_t write;	// aka. head: next idx to write
	uint8_t read;	// aka. tail: next idx to read
	uint8_t full;	// trick to determine whether full when *read == *write
			// using @full is not thread-safe
} ringbuf_t;

int ring_buf_reset(ringbuf_t *ring);
int ring_buf_is_empty(ringbuf_t *ring);
int ring_buf_is_full(ringbuf_t *ring);
int ring_buf_size(ringbuf_t *ring);
int ring_buf_used(ringbuf_t *ring);
void ring_buf_put_override(ringbuf_t *ring, uint8_t data);
int ring_buf_put(ringbuf_t *ring, uint8_t data);
int ring_buf_get(ringbuf_t *ring, uint8_t *data);

#endif /* __RING_BUF_H__ */