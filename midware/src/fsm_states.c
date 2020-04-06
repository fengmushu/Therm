#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "app.h"
#include "app_lcd.h"
#include "app_gpio.h"
#include "app_timer.h"
#include "app_key.h"
#include "app_data.h"
#include "app_fn.h"

#include "fsm.h"

static fsm_state_t state_stop_enter(fsm_node_t *node, fsm_event_t event)
{
    switch (event) {
    case FSM_EVENT_STOP:
        DBG_PRINT("normally exit\n");
        break;

    case __FSM_EVENT_STOP:
        DBG_PRINT("force exit\n");
        break;

    default:
        goto out;
    }

out:
    return node->state;
}

static fsm_node_t state_stop = {
    .state   = FSM_STATE_STOP,
    .type    = FSM_NODE_EXIT,
    .enter   = state_stop_enter,
    .proc    = fsm_dummy_proc,
    .exit    = fsm_dummy_exit,
    .actions = {
        {
            .event  = __FSM_EVENT_NULL,
            .action = NULL,
            .next   = __FSM_STATE_NONE,
        },
    },
};

static fsm_state_t state_pwron_enter(fsm_node_t *node, fsm_event_t event)
{
    fsm_state_t next = node->state;

    UNUSED_PARAM(event);

    if (g_save.calibrated)
        next = FSM_STATE_SLEEP;
    else
        next = FSM_STATE_POST;

    return next;
}

static fsm_node_t state_pwron = {
    .state   = FSM_STATE_PWRON,
    .type    = FSM_NODE_INIT,
    .enter   = state_pwron_enter,
    .proc    = fsm_dummy_proc,
    .exit    = fsm_dummy_exit, 
    .actions = {
        {
            .event  = __FSM_EVENT_NULL,
            .action = NULL,
            .next   = __FSM_STATE_NONE,
        },
    },
};

static fsm_state_t state_pwroff_enter(fsm_node_t *node, fsm_event_t event)
{
    AppLcdClearAll();
    AppLcdSetString(Str_OFF);
    AppLcdDisplayUpdate();
    delay1ms(2000);

    return FSM_STATE_SLEEP;
}

static fsm_node_t state_pwroff = {
    .state   = FSM_STATE_PWROFF,
    .type    = FSM_NODE_NORMAL,
    .enter   = state_pwroff_enter,
    .proc    = fsm_dummy_proc,
    .exit    = fsm_dummy_exit, 
    .actions = {
        {
            .event  = __FSM_EVENT_NULL,
            .action = NULL,
            .next   = __FSM_STATE_NONE,
        },
    },
};

static fsm_state_t state_post_enter(fsm_node_t *node, fsm_event_t event)
{
    fsm_state_t next = node->state;

    UNUSED_PARAM(event);

    // TODO: config check

    AppLedEnable(LedLightBlue);

    AppLcdInit();
    AppLcdDisplayAll();
    AppLcdBlink();

    if (g_save.calibrated)
        next = FSM_STATE_SCAN;
    else
        next = FSM_STATE_FACTORY;

    return next;
}

static fsm_node_t state_post = {
    .state   = FSM_STATE_POST,
    .type    = FSM_NODE_NORMAL,
    .enter   = state_post_enter,
    .proc    = fsm_dummy_proc,
    .exit    = fsm_dummy_exit, 
    .actions = {
        {
            .event  = __FSM_EVENT_NULL,
            .action = NULL,
            .next   = __FSM_STATE_NONE,
        },
    },
};

static fsm_state_t state_sleep_enter(fsm_node_t *node, fsm_event_t event)
{
    fsm_state_t next = node->state;

    UNUSED_PARAM(event);

    // reset lcd
    AppLcdDisplayClear();

    // turn of backlight
    AppLedDisable();

    return next;
}

static fsm_node_t state_sleep = {
    .state   = FSM_STATE_SLEEP,
    .type    = FSM_NODE_NORMAL,
    .enter   = state_sleep_enter,
    .proc    = fsm_dummy_proc,
    .exit    = fsm_dummy_exit, 
    .actions = {
        {
            .event  = FSM_EVENT_PRESS_TRIGGER,
            .action = NULL,
            .next   = FSM_STATE_POST,
        },
        {
            .event  = FSM_EVENT_RELEASE_TRIGGER,
            .action = NULL,
            .next   = FSM_STATE_POST,
        },
        {
            .event  = __FSM_EVENT_NULL,
            .action = NULL,
            .next   = __FSM_STATE_NONE,
        },
    },
};

static fsm_state_t state_main_enter(fsm_node_t *node, fsm_event_t event)
{
    UNUSED_PARAM(event);

    AppLcdClearAll();

    return node->state;
}

static fsm_state_t state_main_proc(fsm_node_t *node)
{
    UNUSED_PARAM(node);

    AppLcdBlink();

    return node->state;
}

static void fn_hold_timer(void *data)
{
    printf("%s: trigger\r\n", __func__);

    // double check
    if (key_released_query(KEY_FN))
        return;

    fsm_event_post(&g_fsm, FSM_EVENT_RING_PRIO_HI, FSM_EVENT_IRQ_TIMER3);
}

static fsm_state_t state_main_press_fn(fsm_node_t *node,
                                       fsm_event_t event,
                                       void *data)
{
    timer3_set(TIM3_PCLK_4M256D_2SEC, 1, fn_hold_timer, NULL);
    timer3_start();

    return node->state;
}

static fsm_state_t state_main_release_fn(fsm_node_t *node,
                                         fsm_event_t event,
                                         void *data)
{
    timer3_stop();

    return node->state;
}

static fsm_node_t state_main = {
    .state   = FSM_STATE_MAIN,
    .type    = FSM_NODE_NORMAL,
    .enter   = state_main_enter,
    .proc    = state_main_proc,
    .exit    = fsm_dummy_exit, 
    .actions = {
        {
            .event  = FSM_EVENT_RELEASE_MINUS,
            .action = NULL,
            .next   = FSM_STATE_MAIN,
        },
        {
            .event  = FSM_EVENT_RELEASE_PLUS,
            .action = NULL,
            .next   = FSM_STATE_MAIN,
        },
        {
            .event  = FSM_EVENT_PRESS_FN,
            .action = state_main_press_fn,
            .next   = FSM_STATE_MAIN,
        },
        {
            .event  = FSM_EVENT_RELEASE_FN,
            .action = state_main_release_fn,
            .next   = FSM_STATE_MAIN,
        },
        {
            .event  = FSM_EVENT_PRESS_TRIGGER,
            .action = NULL,
            .next   = FSM_STATE_SCAN,
        },
        {
            .event  = FSM_EVENT_RELEASE_TRIGGER,
            .action = NULL,
            .next   = FSM_STATE_SCAN,
        },
        {
            .event  = FSM_EVENT_SWITCH_BODY,
            .action = NULL,
            .next   = FSM_STATE_MAIN,
        },
        {
            .event  = FSM_EVENT_SWITCH_SURFACE,
            .action = NULL,
            .next   = FSM_STATE_MAIN,
        },
        {
            .event  = FSM_EVENT_IRQ_TIMER3,
            .action = NULL,
            .next   = FSM_STATE_CONFIG,
        },
        {
            .event  = __FSM_EVENT_NULL,
            .action = NULL,
            .next   = __FSM_STATE_NONE,
        },
    },
};

static fsm_state_t state_scan_enter(fsm_node_t *node, fsm_event_t event)
{
    fsm_state_t next = node->state;

    UNUSED_PARAM(event);

    AppLcdClearAll();
    AppLcdSetLock(TRUE);
    AppLcdDisplayUpdate(); // this thing will clear lcd 

    return next;
}

void state_scan_exit(fsm_node_t *node, fsm_event_t event)
{
    UNUSED_PARAM(node);
    UNUSED_PARAM(event);

    // REVIEW: if we clear lcd in entering MAIN, below can remove
    AppLcdSetLock(FALSE);
    AppLcdDisplayUpdate();
}

static fsm_node_t state_scan = {
    .state   = FSM_STATE_SCAN,
    .type    = FSM_NODE_NORMAL,
    .enter   = state_scan_enter,
    .proc    = fsm_dummy_proc,
    .exit    = state_scan_exit,
    .actions = {
        {
            .event  = FSM_EVENT_IRQ_ADC,
            .action = NULL,
            .next   = FSM_STATE_MAIN,
        },
        {
            .event  = FSM_EVENT_RELEASE_FN,
            .action = NULL,
            .next   = FSM_STATE_MAIN,
        },
        {
            .event  = __FSM_EVENT_NULL,
            .action = NULL,
            .next   = __FSM_STATE_NONE,
        },
    },
};

static fsm_state_t state_config_enter(fsm_node_t *node, fsm_event_t event)
{
    AppLcdClearAll();
    AppLcdDisplayUpdate();

    app_fn_enter();

    return node->state;
}

static fsm_state_t state_config_proc(fsm_node_t *node)
{
    int ret;

    // internal empty shift:
    // according to design, if config is done by btn_fn, goto pwroff
    switch ((ret = app_fn_proc())) {
    case APP_FN_DONE:
        return FSM_STATE_PWROFF;

    case APP_FN_ERR: // TODO
    case APP_FN_OK:
    default:
        break;
    }

    return node->state;
}

static void state_config_exit(fsm_node_t *node, fsm_event_t event)
{
    app_fn_exit();
}

static fsm_state_t state_config_release_fn(fsm_node_t *node,
                                           fsm_event_t event,
                                           void *data)
{
    app_fn_next();
    return node->state;
}

static fsm_state_t state_config_release_minus(fsm_node_t *node,
                                              fsm_event_t event,
                                              void *data)
{
    app_fn_btn_minus();
    return node->state;
}

static fsm_state_t state_config_release_plus(fsm_node_t *node,
                                              fsm_event_t event,
                                              void *data)
{
    app_fn_btn_plus();
    return node->state;
}

static fsm_node_t state_config = {
    .state   = FSM_STATE_CONFIG,
    .type    = FSM_NODE_NORMAL,
    .enter   = state_config_enter,
    .proc    = state_config_proc,
    .exit    = state_config_exit, 
    .actions = {
        {
            .event  = FSM_EVENT_RELEASE_MINUS,
            .action = state_config_release_minus,
            .next   = FSM_STATE_CONFIG,
        },
        {
            .event  = FSM_EVENT_RELEASE_PLUS,
            .action = state_config_release_plus,
            .next   = FSM_STATE_CONFIG,
        },
        {
            .event  = FSM_EVENT_RELEASE_FN,
            .action = state_config_release_fn,
            .next   = FSM_STATE_CONFIG,
        },
        {
            .event  = FSM_EVENT_SWITCH_BODY,
            .action = NULL,
            .next   = FSM_STATE_MAIN,
        },
        {
            .event  = FSM_EVENT_SWITCH_SURFACE,
            .action = NULL,
            .next   = FSM_STATE_MAIN,
        },
        {
            .event  = __FSM_EVENT_NULL,
            .action = NULL,
            .next   = __FSM_STATE_NONE,
        },
    },
};

static fsm_state_t state_factory_enter(fsm_node_t *node, fsm_event_t event)
{
    fsm_state_t next = node->state;

    UNUSED_PARAM(event);

    return next;
}

static fsm_node_t state_factory = {
    .state   = FSM_STATE_FACTORY,
    .type    = FSM_NODE_NORMAL,
    .enter   = state_factory_enter,
    .proc    = fsm_dummy_proc,
    .exit    = fsm_dummy_exit,
    .actions = {
        {
            .event  = __FSM_EVENT_NULL,
            .action = NULL,
            .next   = __FSM_STATE_NONE,
        },
    },
};

static fsm_state_t state_fatal_enter(fsm_node_t *node, fsm_event_t event)
{
    fsm_state_t next = node->state;

    UNUSED_PARAM(event);

    // TODO: LCD: Err

    return next;
}

static fsm_node_t state_fatal = {
    .state   = FSM_STATE_FACTORY,
    .type    = FSM_NODE_EXIT,
    .enter   = state_fatal_enter,
    .proc    = fsm_dummy_proc,
    .exit    = fsm_dummy_exit,
    .actions = {
        {
            .event  = __FSM_EVENT_NULL,
            .action = NULL,
            .next   = __FSM_STATE_NONE,
        },
    },
};

fsm_t g_fsm = {
    .status = FSM_STATUS_UNINIT,
    .curr   = (fsm_node_t *)&state_uninit,

    .states = {
        [FSM_STATE_STOP]        = &state_stop,
        [FSM_STATE_PWRON]       = &state_pwron,
        [FSM_STATE_PWROFF]      = &state_pwroff,
        [FSM_STATE_POST]        = &state_post,
        [FSM_STATE_SLEEP]       = &state_sleep,
        [FSM_STATE_MAIN]        = &state_main,
        [FSM_STATE_SCAN]        = &state_scan,
        [FSM_STATE_CONFIG]      = &state_config,
        [FSM_STATE_FACTORY]     = &state_factory,
        [FSM_STATE_FATAL]       = &state_fatal,
        NULL,
    },
};

int fsm_states_init(fsm_t *fsm)
{
    UNUSED_PARAM(fsm);

    return 0;
}

int fsm_states_exit(fsm_t *fsm)
{
    UNUSED_PARAM(fsm);

    return 0;
}