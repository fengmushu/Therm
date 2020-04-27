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
app_save_t      *g_save;
app_cfg_t       *g_cfg;
scan_log_t      *g_scan_log;

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
    .scan_mode      = SCAN_BODY,
    .sleep_jiffies  = AUTO_SLEEP_TIMEOUT,
};

int app_save_verify(app_save_t *save)
{
    if (save->magic != SAVE_MAGIC)
        return 1;

    for (int i = 0; i < NUM_SCAN_MODES; i++) {
        if (save->scan_log[i].write_idx >= SCAN_LOG_SIZE)
            return 1;

        if (save->scan_log[i].last_write >= SCAN_LOG_SIZE)
            return 1;
    }

    // not allow to change sleep time out now
    if (save->cfg.sleep_jiffies != AUTO_SLEEP_TIMEOUT)
        return 1;

    if (save->cfg.temp_unit >= NUM_TEMP_UNITS)
        return 1;

    if (save->cfg.body_cal_tweak < BODY_CAL_TWEAK_MIN ||
        save->cfg.body_cal_tweak > BODY_CAL_TWEAK_MAX)
        return 1;

    return 0;
}

int app_save_i2c_load(app_save_t *save)
{
    if (factory_mode)
        return 0;

    if (app_i2c_read_data(I2C_DATA_ADDR, (void *)save, sizeof(*save)) == FALSE)
        return 1;

    return 0;
}

int app_save_i2c_store(app_save_t *save)
{
    if (factory_mode)
        return 0;

    if (app_i2c_write_data(I2C_DATA_ADDR, (void *)save, sizeof(*save)) == FALSE)
        return 1;

    return 0;
}

int app_save_i2c_verify_with(app_save_t *save)
{
    app_save_t read;

    if (app_i2c_read_data(I2C_DATA_ADDR, (void *)&read, sizeof(read)) == FALSE)
        return 1;

    if (memcmp(&read, save, sizeof(read)) != 0)
        return 1;

    return 0;
}

void app_save_reset(app_save_t *save)
{
    memset(save, 0x00, sizeof(*save));

    save->magic = SAVE_MAGIC;

    memcpy(&save->cfg, &default_cfg, sizeof(save->cfg));

    if (factory_mode)
        save->cfg.scan_mode = SCAN_SURFACE;
}

void app_runtime_init(app_runtime_t *rt)
{
    memset(rt, 0x00, sizeof(*rt));

    g_rt       = rt;
    g_save     = &rt->save;
    g_cfg      = &rt->save.cfg;
    g_scan_log = rt->save.scan_log;

    g_cal      = AppCalGet();

    rt->battery_lvl = BAT_LVL_HI;

    if (app_save_i2c_load(&rt->save) || app_save_verify(&rt->save)) {
        app_save_reset(&rt->save);
    }

    app_runtime_readidx_rebase(rt);
}

uint8_t scan_mode_runtime_update(void)
{
    return g_cfg->scan_mode;
}

int16_t scan_log_read(scan_log_t *log, uint8_t idx)
{
    if (idx < 0 || idx >= SCAN_LOG_SIZE)
        return -1;

    return log->data[idx];
}

void scan_log_write_safe(scan_log_t *log, int16_t data)
{
    log->last_write = log->write_idx;
    log->data[log->write_idx] = data;

    scan_log_idx_increase(&log->write_idx);
}

void scan_log_write_idx(scan_log_t *log, uint8_t idx, int16_t data)
{
    log->last_write = idx;
    log->data[idx]  = data;
}
