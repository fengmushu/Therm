#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"

#include "ring_buf.h"

int ring_buf_reset(ringbuf_t *ring)
{
    if (!ring)
        return 1;

    memset(ring, 0x00, sizeof(*ring));
    return 0;
}

int ring_buf_is_empty(ringbuf_t *ring)
{
    if (!ring->full && (ring->write == ring->read))
        return 1;

    return 0;
}

int ring_buf_is_full(ringbuf_t *ring)
{
    return ring->full;
}

int ring_buf_size(ringbuf_t *ring)
{
    // static alloc
    return sizeof(ring->buf);
}

int ring_buf_used(ringbuf_t *ring)
{
    int size = ring_buf_size(ring);

    if (!ring->full) {
        if (ring->write >= ring->read) {
            size = ring->write - ring->read;
        } else {
            size = ring->write + (size - ring->read);
        }
    }

    return size;
}

// if using this, @full is not always true
void ring_buf_put_override(ringbuf_t *ring, uint8_t data)
{
    ring->buf[ring->write] = data;
    ring->write = RING_NEXT_IDX(ring->write);
    ring->full = (ring->write == ring->read);
}

int ring_buf_put(ringbuf_t *ring, uint8_t data)
{
    if (ring_buf_is_full(ring))
        return 1;

    __disable_irq();
    ring_buf_put_override(ring, data);
    __enable_irq();

    return 0;
}

int ring_buf_get(ringbuf_t *ring, uint8_t *data)
{
    if (ring_buf_is_empty(ring))
        return 1;

    __disable_irq();
    *data = ring->buf[ring->read];
    ring->read = RING_NEXT_IDX(ring->read);
    ring->full = 0;
    __enable_irq();

    return 0;
}
