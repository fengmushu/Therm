
#ifndef __APP_MAIN_H__
#define __APP_MAIN_H__

#include "fsm.h"

fsm_state_t state_main_proc(fsm_node_t *node);
fsm_state_t state_main_enter(fsm_node_t *node, fsm_event_t event);
void state_main_exit(fsm_node_t *node, fsm_event_t event);

fsm_state_t state_main_release_minus(fsm_node_t *node, fsm_event_t event, void *data);
fsm_state_t state_main_release_plus(fsm_node_t *node, fsm_event_t event, void *data);

fsm_state_t state_main_press_fn(fsm_node_t *node, fsm_event_t event, void *data);
fsm_state_t state_main_release_fn(fsm_node_t *node, fsm_event_t event, void *data);

fsm_state_t state_main_scan_mode_switch(fsm_node_t *node, fsm_event_t event, void *data);

#endif /* __APP_MAIN_H__ */