#ifndef __FSM_H__
#define __FSM_H__

#include "ring_buf.h"

// @f must be pointer to fsm
#define fsm_state_to_node(f, i)         ((f)->states[i])
#define is_fsm_uninit(f)                ((f)->status == FSM_STATUS_UNINIT)
#define is_fsm_stopped(f)               ((f)->status == FSM_STATUS_STOPPED)
#define is_fsm_running(f)               ((f)->status == FSM_STATUS_RUNNING)
#define is_fsm_shifting(f)              ((f)->status == FSM_STATUS_SHIFTING)

#define is_fsm_in_states(f, bm)         (BIT((f)->curr->state) & (bm))

typedef struct fsm fsm_t;
typedef struct fsm_node fsm_node_t;
typedef struct fsm_handler fsm_handler_t;

typedef enum fsm_event_ring fsm_event_ring_t;
typedef enum fsm_event fsm_event_t, FSM_EVENT;
typedef enum fsm_state fsm_state_t, FSM_STATE;
typedef enum fsm_node_type fsm_node_type_t;
typedef enum fsm_status fsm_status_t;
typedef enum fsm_event_accept fsm_event_accept_t;

enum fsm_event_ring {
        FSM_EVENT_RING_PRIO_HI = 0,
        FSM_EVENT_RING_PRIO_LO,
        NUM_FSM_EVENT_RINGS,
};

/*
 * remember to check string list,
 * while modifying this list.
 */
enum fsm_event {
        FSM_EVENT_STOP = 0,

        FSM_EVENT_PRESS_TRIGGER,
        FSM_EVENT_PRESS_MINUS,
        FSM_EVENT_PRESS_PLUS,
        FSM_EVENT_PRESS_FN,

        FSM_EVENT_RELEASE_TRIGGER,
        FSM_EVENT_RELEASE_MINUS,
        FSM_EVENT_RELEASE_PLUS,
        FSM_EVENT_RELEASE_FN,

        FSM_EVENT_SWITCH_BODY,
        FSM_EVENT_SWITCH_SURFACE,

        FSM_EVENT_IRQ_TIMER3,
        FSM_EVENT_IRQ_ADC,
        FSM_EVENT_IRQ_RTC,
        FSM_EVENT_IRQ_LVD,

        NUM_FSM_EVENTS,         // user event count

        __FSM_EVENT_ANY,        // empty shift, not counted
        __FSM_EVENT_NULL,       // termination flag
        __FSM_EVENT_DUMMY,      // do nothing
        __FSM_EVENT_START,      // fsm is up, for first init code in init state
        __FSM_EVENT_STOP,       // fsm is going down by force

        NUM_FSM_ALL_EVENTS,
};

enum fsm_state {
        FSM_STATE_STOP = 0,

        FSM_STATE_PWRON,
        FSM_STATE_PWROFF,
        FSM_STATE_POST,
        FSM_STATE_SLEEP,
        FSM_STATE_MAIN,
        FSM_STATE_SCAN,
        FSM_STATE_CONFIG,
        FSM_STATE_FACTORY,
        FSM_STATE_FATAL,

        NUM_FSM_STATES,         // user state count

        __FSM_STATE_NONE,       // next state is not valid (negative return)
                                // may use in handler actions to reject event

        __FSM_STATE_OK,         // dummy positive return for pre/post actions
                                // will be treat as __FSM_STATE_SELF
                                // if it's returned at last

        __FSM_STATE_SELF,       // no transition will happend
                                // handler->action can be null
                                // use this within handler->next field

        __FSM_STATE_BY_NEXT,    // next state is handler->next,
                                // use this within handler->action field

        __FSM_STATE_UNINIT,     // not initialized

        NUM_FSM_ALL_STATES,
};

enum fsm_node_type {
        FSM_NODE_INIT = 0,
        FSM_NODE_DUMMY,
        FSM_NODE_NORMAL,
        FSM_NODE_EXIT,

        NUM_FSM_NODE_TYPE,
};

enum fsm_event_accept {
        FSM_ACCEPTED = 0,         // event accepted
        FSM_UNACCEPTED,           // event unaccepted
};

enum fsm_status {
        FSM_STATUS_UNINIT = 0,
        FSM_STATUS_RUNNING,
        FSM_STATUS_SHIFTING,
        FSM_STATUS_STOPPED,
        NUM_FSM_STATUS,
};

/**
 *      @event:         event this handler defined to handle
 *      @action:        complexe handler
 *      @next:          many events only needs a simple goto statement handler,
 *                      use this instead of writing many handlers.
 *                      if @action is NULL, @next is not __FSM_STATE_NONE,
 *                      fsm will goto @next.
 */
struct fsm_handler {
        // event for current handler to handle
        fsm_event_t     event;

        // default next state if handler->action is null
        fsm_state_t     next;

        // callback executed before handler->action
        // @return:     1. __FSM_STATE_NONE to reject event
        //              2. __FSM_STATE_OK to continue to handler->action
        fsm_state_t     (*pre)(fsm_node_t *, fsm_event_t, void *);

        // @fsm_node_t:  current node that handling event
        // @fsm_event_t: event now handling
        // @void *:      private data provided by fsm_event_input()
        // @return:      1. next state to goto
        //               2. __FSM_STATE_NONE to reject, handle error
        fsm_state_t     (*action)(fsm_node_t *, fsm_event_t, void *);

        // callback executed after handler->action
        // @return:     1. __FSM_STATE_NONE to reject event
        //              2. other states to goto
        fsm_state_t     (*post)(fsm_node_t *, fsm_event_t, void *);
};

/**
 *      @state:         related state enum.
 *      @type:          type of current state.
 *      @enter:         return current state for stay,
 *                      other states for empty shift.
 *      @proc:          main function for this state, will loop by fsm
 *      @exit:          callback when exit current state.
 *      @actions:       flex list of accepted event and its handlers,
 *                      MUST be terminated with NULL flag.
 */
struct fsm_node {
        fsm_state_t     state;
        fsm_node_type_t type;

        // @fsm_node_t:  node that will enter (this node)
        // @fsm_event_t: event that causes to enter
        // @return:      can perform empty shift
        fsm_state_t     (*enter)(fsm_node_t *, fsm_event_t);

        // @fsm_node_t:  node that will enter (this node)
        // @return:      node to shift to
        fsm_state_t     (*proc)(fsm_node_t *);

        // @fsm_node_t:  node that will leave (this node)
        // @fsm_event_t: event that causes to exit
        void            (*exit)(fsm_node_t *, fsm_event_t);

        // need terminate flag
        fsm_handler_t    actions[];
};

/**
 *      @status:        current running state of fsm
 *      @curr:          current state of fsm
 *      @states:        flex list of available states,
 *                      MUST be mapped to state enum sequence;
 *                      MUST be terminated with NULL pointer.
 */
struct fsm {
        ringbuf_t       events[NUM_FSM_EVENT_RINGS];

        fsm_status_t    status;
        fsm_node_t      *curr;

        fsm_node_t      *states[];      // at least put a NULL in list!
};

extern const fsm_node_t state_uninit;
extern fsm_t g_fsm;

fsm_state_t fsm_dummy_enter(fsm_node_t *node, fsm_event_t event);
void fsm_dummy_exit(fsm_node_t *node, fsm_event_t event);
fsm_state_t fsm_dummy_proc(fsm_node_t *node);

int fsm_state_enter(fsm_t *fsm, fsm_event_t event, fsm_node_t *next);
int fsm_event_input(fsm_t *fsm, fsm_event_t event, void *data);

int fsm_event_post(fsm_t *fsm, fsm_event_ring_t ring, fsm_event_t event);
int fsm_event_process(fsm_t *fsm);

int fsm_start(fsm_t *fsm);
int fsm_shutdown(fsm_t *fsm, fsm_state_t stop);
int fsm_process(fsm_t *fsm);

int fsm_init(fsm_t *fsm);
int fsm_exit(fsm_t *fsm);
int fsm_states_init(fsm_t *fsm);
int fsm_states_exit(fsm_t *fsm);

int __fsm_event_input(fsm_t *fsm, fsm_event_t event, void *data);

#endif /* __FSM_H__ */