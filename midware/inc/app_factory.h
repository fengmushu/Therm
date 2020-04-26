#ifndef __APP_FACTORY_H__
#define __APP_FACTORY_H__

#include <stdint.h>

#include "app_data.h"

// #define FACTORY_MODE_UV_DEBUG

extern uint8_t factory_mode;

#ifdef FACTORY_MODE_UV_DEBUG
extern uint8_t    log_show_uv;
extern uint32_t   last_uv, last_ntc;
extern scan_log_t log_uv[NUM_SCAN_MODES];
extern scan_log_t log_ntc[NUM_SCAN_MODES];
#endif

void factory_test(void);

#endif /* __APP_FACTORY_H__ */