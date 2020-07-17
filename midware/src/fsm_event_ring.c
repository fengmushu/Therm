#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"

#include "fsm_event_ring.h"

int event_ring_reset(event_ring_t *ring)
{
        if (!ring)
                return 1;

        memset(ring, 0x00, sizeof(*ring));
        return 0;
}

int event_ring_is_empty(event_ring_t *ring)
{
        if (!ring->full && (ring->write == ring->read))
                return 1;

        return 0;
}

int event_ring_is_full(event_ring_t *ring)
{
        return ring->full;
}

int event_ring_size(event_ring_t *ring)
{
        // static alloc
        return sizeof(ring->buf);
}

int event_ring_used(event_ring_t *ring)
{
        int size = event_ring_size(ring);

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
void event_ring_put_override(event_ring_t *ring, event_ring_data_t *data)
{
        memcpy(&ring->buf[ring->write], data, sizeof(event_ring_data_t));
        ring->write = RING_NEXT_IDX(ring->write);
        ring->full = (ring->write == ring->read);
}

int event_ring_put(event_ring_t *ring, event_ring_data_t *data)
{
        if (event_ring_is_full(ring))
                return 1;

        __disable_irq();
        event_ring_put_override(ring, data);
        __enable_irq();

        return 0;
}

int event_ring_get(event_ring_t *ring, event_ring_data_t *data)
{
        if (event_ring_is_empty(ring))
                return 1;

        __disable_irq();
        memcpy(data, &ring->buf[ring->read], sizeof(event_ring_data_t));
        ring->read = RING_NEXT_IDX(ring->read);
        ring->full = 0;
        __enable_irq();

        return 0;
}
