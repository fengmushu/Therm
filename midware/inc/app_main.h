
#ifndef __APP_MAIN_H__
#define __APP_MAIN_H__

#include "fsm.h"

fsm_state_t state_main_proc(fsm_node_t *node, fsm_event_t *out);
fsm_state_t state_main_enter(fsm_node_t *node, fsm_event_t event);
void state_main_exit(fsm_node_t *node, fsm_event_t event);

#endif /* __APP_MAIN_H__ */