#include <stdint.h>
#include <string.h>

#include "app.h"
#include "app_data.h"
#include "app_i2c.h"
#include "app_key.h"
#include "app_lcd.h"
#include "app_factory.h"

app_runtime_t    g_runtime;
app_runtime_t   *g_rt;
app_cfg_t       *g_cfg;

CalData_t       *g_cal;

temp_thres_t g_temp_thres[NUM_SCAN_MODES] = {
    [SCAN_BODY]    = { BODY_TEMP_UNDERFLOW_C,    BODY_TEMP_OVERFLOW_C    },
    [SCAN_SURFACE] = { SURFACE_TEMP_UNDERFLOW_C, SURFACE_TEMP_OVERFLOW_C },
};

static app_cfg_t default_cfg = {
    .body_cal_tweak = 0,
    .body_alarm_C   = BODY_ALARM_THRESH_DEF,
    .temp_unit      = TUNIT_C,
    .beep_on        = BEEP_ON,
    .sleep_jiffies  = AUTO_SLEEP_TIMEOUT,
};

void app_runtime_init(app_runtime_t *rt)
{
    memset(rt, 0x00, sizeof(*rt));
}

uint8_t scan_mode_runtime_update(void)
{
    return g_rt->scan_mode;
}
