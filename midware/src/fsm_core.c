#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "fsm_event_ring.h"
#include "fsm.h"
#include "app.h"
#include "app_key.h"

#ifndef FSM_DBG_PRINT
#undef DBG_PRINT
#define DBG_PRINT(x, ...) do { } while(0)
#endif

// XXX: to handle null dereference at start
const fsm_node_t state_uninit = {
        .state   = __FSM_STATE_UNINIT,
        .type    = FSM_NODE_DUMMY,
        .enter   = NULL,
        .proc    = NULL,
        .exit    = NULL,
        .events  = 0,
        .actions = {
                {
                        .event  = __FSM_EVENT_NULL,
                        .action = NULL,
                },
        },
};

fsm_state_t fsm_dummy_enter(fsm_node_t *node, fsm_event_t event)
{
        UNUSED_PARAM(event);

        // return other states for empty shifting
        return node->state;
}

void fsm_dummy_exit(fsm_node_t *node, fsm_event_t event)
{
        UNUSED_PARAM(node);
        UNUSED_PARAM(event);
}

fsm_state_t fsm_dummy_proc(fsm_node_t *node, fsm_event_t *out)
{
        UNUSED_PARAM(out);

        return node->state;
}

fsm_node_t *fsm_init_state_get(fsm_t *fsm)
{
        fsm_node_t *ret = NULL;
        fsm_node_t *n = NULL;
        int i = 0;

        while (i < NUM_FSM_STATES) {
                n = fsm_state_to_node(fsm, i);
                i++;

                // in case there are holes in state list
                if (!n)
                        continue;

                if (n->type == FSM_NODE_INIT) {
                        if (ret == NULL) {
                                ret = n;
                        } else {
                                DBG_PRINT("more than one init state found!\n");
                                return NULL;
                        }
                }
        }

        return ret;
}

fsm_state_t fsm_event_handle(fsm_t *fsm, fsm_event_t event, void *data)
{
        fsm_node_t *curr;
        fsm_handler_t *handler;
        fsm_state_t next = __FSM_STATE_NONE;
        int i;

        i = 0;
        curr = fsm->curr;
        handler = &curr->actions[i];

        while (handler->event != __FSM_EVENT_NULL) {
                if (handler->event == event) {
                        if (handler->action) {
                                next = handler->action(curr, event, data);

                                // if (next == __FSM_STATE_BY_NEXT)
                                //     next = handler->next;
                        } else {
                                next = handler->next;
                        }

                        break;
                }

                handler = &curr->actions[++i];
        }

        return next;
}

static __always_inline fsm_state_t __fsm_state_enter(fsm_t *fsm, fsm_event_t event, fsm_node_t *next)
{
        fsm_node_t *curr = fsm->curr;

        if (curr && curr->exit)
                curr->exit(curr, event);

        fsm->curr = next;

        // next should not be NULL here
        if (next->enter)
                return next->enter(next, event);

        return next->state;
}

int fsm_state_enter(fsm_t *fsm, fsm_event_t event, fsm_node_t *next)
{
        fsm_state_t next_state;

        fsm->status = FSM_STATUS_SHIFTING;

        // perform multiple empty shift here
        do {
                next_state = __fsm_state_enter(fsm, event, next);
                next = fsm_state_to_node(fsm, next_state);
        } while (next != fsm->curr);

        fsm->status = FSM_STATUS_RUNNING;

        // for startup, fsm->status is set to running after fsm_state_enter()
        if (fsm->curr->type == FSM_NODE_EXIT)
                fsm->status = FSM_STATUS_STOPPED;

        return 0;
}

static __always_inline int __fsm_event_input(fsm_t *fsm, fsm_event_t event, void *data)
{
        fsm_state_t ret;

        if (is_fsm_stopped(fsm) || is_fsm_uninit(fsm))
                return FSM_UNACCEPTED;

        if (!(BIT(event) & fsm->curr->events))
                return FSM_UNACCEPTED;

        if (event == __FSM_EVENT_DUMMY)
                return FSM_ACCEPTED;

        ret = fsm_event_handle(fsm, event, data);

        switch (ret) {
        case __FSM_STATE_OK:
        case __FSM_STATE_SELF:
                return FSM_ACCEPTED;

        case __FSM_STATE_NONE:
                return FSM_UNACCEPTED;

        default:
                break;
        }

        // we need to transit to another state
        if (ret != fsm->curr->state) {
                fsm_node_t *next = fsm_state_to_node(fsm, ret);
                fsm_state_enter(fsm, event, next);
        }

        return FSM_ACCEPTED;
}

/**
 * fsm_event_input() - block waiting to input fsm event
 *
 * @fsm:        pointer to fsm
 * @event:      event to input
 * @data:       data to pass to action handler
 *
 * return 0 on success, otherwise failure
 */
int fsm_event_input(fsm_t *fsm, fsm_event_t event, void *data)
{
        int ret;

        if (!fsm) {
                DBG_PRINT("invalid pointer to fsm\n");
                return FSM_UNACCEPTED;
        }

        ret = __fsm_event_input(fsm, event, data);

        return ret;
}

int fsm_event_post(fsm_t *fsm, fsm_event_ring_type_t type, fsm_event_t event)
{
        event_ring_t *ring;
        event_ring_data_t ring_data;

        if (is_fsm_stopped(fsm) || is_fsm_uninit(fsm))
                return 1;

        if (type < 0 || type >= NUM_FSM_EVENT_RINGS)
                return 1;

        if (type == FSM_EVENT_RING_PRIO_LO && is_fsm_shifting(fsm))
                return 1;

        ring = &fsm->events[type];

        if (event_ring_is_full(ring)) {
                DBG_PRINT("event ring full\r\n");
                return 1;
        }

        ring_data.event = (uint8_t)event;
        ring_data.state = (uint8_t)fsm->curr->state;

        event_ring_put(ring, &ring_data);

        return 0;
}

static __always_inline int __fsm_event_process(fsm_t *fsm, event_ring_t *ring)
{
        event_ring_data_t ring_data;

        while (!event_ring_is_empty(ring)) {
                if (event_ring_get(ring, &ring_data))
                        return 1;

                DBG_PRINT("event_process: event->state %d->%d\r\n",
                                  ring_data.event, ring_data.state);

                if (fsm->curr->state != ring_data.state) {
                        DBG_PRINT("state shifted, abort\r\n");
                        continue;
                }

                // perform state transition, atomic,
                // will not disturb by new event
                fsm_event_input(fsm, ring_data.event, NULL);
        }

        return 0;
}

int fsm_event_process(fsm_t *fsm)
{
        for (int i = 0; i < NUM_FSM_EVENT_RINGS; i++) {
                if (__fsm_event_process(fsm, &fsm->events[i]))
                        return 1;
        }

        return 0;
}

int fsm_init(fsm_t *fsm)
{
        int err;

        DBG_PRINT("enter\n");

        err = fsm_states_init(fsm);
        if (err)
                return err;

        for (int i = 0; i < NUM_FSM_EVENT_RINGS; i++) {
                err = event_ring_reset(&fsm->events[i]);
                if (err)
                        return err;
        }

        return 0;
}

int fsm_exit(fsm_t *fsm)
{
        int err;

        DBG_PRINT("enter\n");

        err = fsm_states_exit(fsm);
        if (err)
                return err;

        return 0;
}

int fsm_start(fsm_t *fsm)
{
        fsm_node_t *init;

        if (!fsm)
                return -EINVAL;

        if (!is_fsm_uninit(fsm)) {
                DBG_PRINT("fsm is inited\n");
                return 1;
        }

        init = fsm_init_state_get(fsm);
        if (!init) {
                DBG_PRINT("no init state found\n");
                return 1;
        }

        if (NUM_FSM_EVENTS >= (BITS_PER_BYTE * sizeof(init->events)))
                return 1;

        if (fsm_state_enter(fsm, __FSM_EVENT_START, init))
                return 1;

        fsm->status = FSM_STATUS_RUNNING;

        return 0;
}

int fsm_process(fsm_t *fsm)
{
        fsm_state_t ret;
        fsm_event_t event;

        while (!is_fsm_stopped(fsm)) {
                event = __FSM_EVENT_ANY;
                ret = __FSM_STATE_NONE;

                if (fsm_event_process(fsm))
                        return 1;

                if (is_fsm_stopped(fsm))
                        break;

                if (fsm->curr->proc)
                        ret = fsm->curr->proc(fsm->curr, &event);

                // state proc() require to empty shift to another
                if (ret != fsm->curr->state && ret != __FSM_STATE_NONE) {
                        fsm_node_t *next = fsm_state_to_node(fsm, ret);

                        if (fsm_state_enter(fsm, event, next))
                                return 1;
                }
        }

        return 0;
}

/**
 * fsm_shutdown() - halt fsm by force
 *
 * try input termination event first,
 * if not accepted by fsm, use this
 * to force to goto terminate state.
 *
 * @fsm:        pointer to fsm instance
 * @stop:       termination state to goto
 *
 * return 0 on success, otherwise errno
 */
int fsm_shutdown(fsm_t *fsm, fsm_state_t stop)
{
        fsm_node_t *stop_node;
        int err = 0;

        if (!fsm)
                return -EINVAL;

        if (!is_fsm_running(fsm)) {
                DBG_PRINT("fsm is not running\n");
                return 1;
        }

        stop_node = fsm_state_to_node(fsm, stop);
        if (!stop_node || stop_node->type != FSM_NODE_EXIT) {
                DBG_PRINT("ivnalied stop state\n");
                return -EINVAL;
        }

        // fsm->status is set in fsm_state_enter()
        if (fsm_state_enter(fsm, __FSM_EVENT_STOP, stop_node)) {
                DBG_PRINT("failed to enter termination state\n");
                err = 1;
        }

        return err;
}
