#include <stdint.h>
#include <string.h>

#include "app.h"
#include "app_data.h"
#include "app_i2c.h"
#include "app_key.h"
#include "app_lcd.h"

app_runtime_t    g_runtime;
app_runtime_t   *g_rt;
app_save_t      *g_save;
app_cfg_t       *g_cfg;
scan_log_t      *g_scan_log;

CalData_t       *g_cal;

temp_thres_t g_temp_thres[NUM_SCAN_MODES] = {
    [SCAN_BODY]    = { BODY_TEMP_LOW_THRES,    BODY_TEMP_HI_THRES    },
    [SCAN_SURFACE] = { SURFACE_TEMP_LOW_THRES, SURFACE_TEMP_HI_THRES },
};

static app_cfg_t default_cfg = {
    .cal_offset    = 0,
    .body_alarm_C  = BODY_ALARM_THRESH_DEF,
    .temp_unit     = TUNIT_C,
    .beep_on       = BEEP_ON,
    .sleep_jiffies = AUTO_SLEEP_TIMEOUT,
};

int app_save_verify(app_save_t *save)
{
    if (save->magic != SAVE_MAGIC)
        return 1;

    return 0;
}

int app_save_i2c_load(app_save_t *save)
{
    if (app_i2c_read_data(I2C_DATA_ADDR, (void *)save, sizeof(*save)) == FALSE)
        return 1;

    return 0;
}

int app_save_i2c_store(app_save_t *save)
{
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
}

void app_runtime_init(app_runtime_t *rt)
{
    memset(rt, 0x00, sizeof(*rt));

    g_rt       = rt;
    g_save     = &rt->save;
    g_cfg      = &rt->save.cfg;
    g_scan_log = rt->save.scan_log;

    g_cal      = AppCalGet();

    if (app_save_i2c_load(&rt->save) || app_save_verify(&rt->save)) {
        app_save_reset(&rt->save);
    }

    for (int i = 0; i < NUM_SCAN_MODES; i++)
        rt->read_idx[i] = rt->save.scan_log[i].write_idx;
}

uint8_t scan_mode_runtime_update(void)
{
    if (key_released_query(KEY_SWITCH))
        g_rt->scan_mode = SCAN_BODY;
    else
        g_rt->scan_mode = SCAN_SURFACE;

    return g_rt->scan_mode;
}

int16_t scan_log_read(scan_log_t *log, uint8_t idx)
{
    if (idx < 0 || idx >= SCAN_LOG_SIZE)
        return -1;

    return log->data[idx];
}

void scan_log_write(scan_log_t *log, int16_t data)
{
    log->last_write = log->write_idx;
    log->data[log->write_idx++] = data;
}
