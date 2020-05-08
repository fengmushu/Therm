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
#include "app_main.h"
#include "app_cal.h"
#include "app_adc.h"
#include "app_rtc.h"
#include "app_factory.h"

#include "fsm.h"

void sys_resume(void)
{
    AppLcdEnable();

    AppLedEnable(LedGreen);

    AppMAdcPowerOn();
    Adc_Enable();
    Bgr_BgrEnable();

    Rtc_Cmd(TRUE);

    Lvd_Enable();
}

void sys_halt(void)
{
    Lvd_Disable();

    // turn off rtc counter
    Rtc_Cmd(FALSE);

    // beep on, in case
    beep_off();

    // reset lcd
    AppLcdDisplayClear();

    // turn off backlight
    AppLedDisable();

    // turn off lcd
    AppLcdClearAll();
    AppLcdDisplayUpdate(0);
    AppLcdDisable();

    // turn off adc
    AppMAdcPowerOff();
    Adc_Disable();
    Bgr_BgrDisable();

    // go deep sleep
    Lpm_GotoLpmMode();
}

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
    .proc    = NULL,
    .exit    = NULL,
    .events  = 0,
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
    UNUSED_PARAM(event);

    beep_once(225);

    // AppLedEnable(LedOrange);

    // AppLcdClearAll();
    // AppLcdDisplayClear();
    // AppLcdDisplayAll();
    // delay1ms(2500);

    return FSM_STATE_SLEEP;
}

static fsm_node_t state_pwron = {
    .state   = FSM_STATE_PWRON,
    .type    = FSM_NODE_INIT,
    .enter   = state_pwron_enter,
    .proc    = NULL,
    .exit    = NULL,
    .events  = 0,
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

    if (app_save_i2c_store(g_save))
        DBG_PRINT("failed to save data to i2c\r\n");

    AppLedEnable(LedOrange);

    AppLcdClearAll();
    AppLcdSetString(Str_OFF);
    AppLcdDisplayUpdate(2000);

    return FSM_STATE_SLEEP;
}

static fsm_node_t state_pwroff = {
    .state   = FSM_STATE_PWROFF,
    .type    = FSM_NODE_NORMAL,
    .enter   = state_pwroff_enter,
    .proc    = NULL,
    .exit    = NULL,
    .events  = 0,
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
    UNUSED_PARAM(event);

    sys_halt();

    return node->state;
}

static void state_sleep_exit(fsm_node_t *node, fsm_event_t event)
{
    UNUSED_PARAM(node);
    UNUSED_PARAM(event);

    sys_resume();

    // hold [beep] and [fn] to reset, clear config
    if (key_pressed_query(KEY_BEEP) && key_pressed_query(KEY_FN)) {
        AppLcdBlink();
        app_save_reset(g_save);
        __app_save_i2c_store(g_save, 1);
        app_runtime_readidx_rebase(g_rt);
    }

    //
    // NOTE: when system wakes up from deep sleep
    //       adc/bgr needs a little delay to settle down
    //       otherwise the first sampling may be werid
    //
    AppLcdDisplayClear();
    AppLcdDisplayAll();
    delay1ms(1000);

    AppLcdDisplayClear();
    AppLcdClearAll();
    AppLcdDisplayUpdate(0);
}

static fsm_state_t state_sleep_proc(fsm_node_t *node, fsm_event_t *out)
{
    fsm_state_t next = node->state;

    if (key_pressed_query(KEY_TRIGGER))
        next = FSM_STATE_SCAN;

    return next;
}

static fsm_node_t state_sleep = {
    .state   = FSM_STATE_SLEEP,
    .type    = FSM_NODE_NORMAL,
    .enter   = state_sleep_enter,
    .proc    = state_sleep_proc,
    .exit    = state_sleep_exit,
    .events  = 0,
    .actions = {
        {
            .event  = __FSM_EVENT_NULL,
            .action = NULL,
            .next   = __FSM_STATE_NONE,
        },
    },
};

static fsm_node_t state_main = {
    .state   = FSM_STATE_MAIN,
    .type    = FSM_NODE_NORMAL,
    .enter   = state_main_enter,
    .proc    = state_main_proc,
    .exit    = state_main_exit,
    .events  = 0,
    .actions = {
        {
            .event  = FSM_EVENT_RELEASE_BEEP,
            .action = fsm_state_beep_cycle,
            .next   = __FSM_STATE_NONE,
        },
        {
            .event  = FSM_EVENT_RELEASE_FN,
            .action = NULL,
            .next   = FSM_STATE_CONFIG,
        },
        // {
        //     .event  = FSM_EVENT_IRQ_TIMER3,
        //     .action = NULL,
        //     .next   = FSM_STATE_CONFIG,
        // },
        {
            .event  = FSM_EVENT_SYS_HALT,
            .action = NULL,
            .next   = FSM_STATE_PWROFF,
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

    // display [----] will override last result
    // which will break burst scan mode
    // AppLcdSetString(Str_LINE);
    // AppLcdDisplayUpdate(0);

    return next;
}

// NOTE: least 2 digit is float .2, delta = .XX
int16_t markov_chain_trick(int16_t previous, int16_t current, uint16_t delta)
{
    int16_t ret = previous;

    // if current is not in range (previous +- delta), return current
    if (abs(previous - current) > (delta * 2))
        ret = current;

    return ret;
}

static fsm_state_t state_scan_proc(fsm_node_t *node, fsm_event_t *out)
{
    uint32_t result[NUM_SCAN_MODES];
    uint32_t uv, ntc;

    // prevent sleep
    AppRtcFeed();

    // AppTempCalculate() return least 2 digit as float points
    AppTempCalculate(g_cal, &ntc, &result[SCAN_SURFACE], &result[SCAN_BODY], &uv);

    for (uint8_t i = 0; i < NUM_SCAN_MODES; i++) {
        int16_t last_written = scan_log_last_written(&g_scan_log[i]);
        g_rt->scan_result[i] = markov_chain_trick(last_written, result[i], 5);
        g_rt->scan_result[i] /= 10; // display goes with one float digit
    }

#ifdef FACTORY_MODE_UV_DEBUG
    if (factory_mode) {
        last_uv = uv;
        last_ntc = ntc / 100;
    }
#endif

    state_proc_event_set(out, FSM_EVENT_SCAN_DONE);

    return FSM_STATE_MAIN;
}

void state_scan_exit(fsm_node_t *node, fsm_event_t event)
{
    UNUSED_PARAM(node);
    UNUSED_PARAM(event);
}

static fsm_node_t state_scan = {
    .state   = FSM_STATE_SCAN,
    .type    = FSM_NODE_NORMAL,
    .enter   = state_scan_enter,
    .proc    = state_scan_proc,
    .exit    = state_scan_exit,
    .events  = 0,
    .actions = {
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
    AppLcdDisplayUpdate(30);

    app_fn_enter();

    return node->state;
}

static fsm_state_t state_config_proc(fsm_node_t *node, fsm_event_t *out)
{
    int ret;

    switch ((ret = app_fn_proc())) {
    case APP_FN_DONE:
        return FSM_STATE_MAIN; // fsm internal empty shift

    case APP_FN_ERR: // TODO: lcd show Err..
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

static fsm_node_t state_config = {
    .state   = FSM_STATE_CONFIG,
    .type    = FSM_NODE_NORMAL,
    .enter   = state_config_enter,
    .proc    = state_config_proc,
    .exit    = state_config_exit, 
    .events  = 0,
    .actions = {
        // {
        //     .event  = FSM_EVENT_RELEASE_BEEP,
        //     .action = fsm_state_beep_cycle,
        //     .next   = __FSM_STATE_NONE,
        // },
        {
            .event  = FSM_EVENT_RELEASE_TRIGGER,
            .action = NULL,
            .next   = FSM_STATE_MAIN,
        },
        {
            .event  = FSM_EVENT_SYS_HALT,
            .action = NULL,
            .next   = FSM_STATE_PWROFF,
        },
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
    .state   = FSM_STATE_FATAL,
    .type    = FSM_NODE_EXIT,
    .enter   = state_fatal_enter,
    .proc    = NULL,
    .exit    = NULL,
    .events  = 0,
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
        [FSM_STATE_SLEEP]       = &state_sleep,
        [FSM_STATE_MAIN]        = &state_main,
        [FSM_STATE_SCAN]        = &state_scan,
        [FSM_STATE_CONFIG]      = &state_config,
        [FSM_STATE_FATAL]       = &state_fatal,
        NULL,
    },
};

void fsm_event_bitmap_mark(fsm_t *fsm)
{
    fsm_node_t *n = NULL;
    fsm_handler_t *handler = NULL;
    int i = 0;

    while (i < NUM_FSM_STATES) {
        n = fsm_state_to_node(fsm, i);
        i++;

        // in case there are holes in state list
        if (!n)
            continue;

        handler = &n->actions[0];
        while (handler->event != __FSM_EVENT_NULL) {
            n->events |= BIT(handler->event);
            handler++;
        }
    }
}


int fsm_states_init(fsm_t *fsm)
{
    fsm_event_bitmap_mark(fsm);

    return 0;
}

int fsm_states_exit(fsm_t *fsm)
{
    UNUSED_PARAM(fsm);

    return 0;
}