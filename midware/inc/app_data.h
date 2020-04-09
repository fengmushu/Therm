#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <stdint.h>

#include "app_cal.h"
#include "utils.h"

#define BODY_TEMP_UNDERFLOW_C       (300)   // show LO
#define BODY_TEMP_OVERFLOW_C        (440)   // show HI

#define SURFACE_TEMP_UNDERFLOW_C    (0)
#define SURFACE_TEMP_OVERFLOW_C     (1000)

#define BODY_ALARM_THRESH_MIN       (371)
#define BODY_ALARM_THRESH_DEF       (380)
#define BODY_ALARM_THRESH_MAX       (386)

#define BODY_FEVER_THRS             (371)   // =37.1 ~ =37.2
#define BODY_FEVER_LOW              (373)   // =37.3 ~ =38.0
#define BODY_FEVER_MID              (381)   // =38.1 ~ =39.0
#define BODY_FEVER_HIGH             (391)   // =39.1 ~ =40.0
#define BODY_FEVER_DEAD             (410)   // =41.0 ~

#define BODY_CAL_TWEAK_MIN          (-30)
#define BODY_CAL_TWEAK_MAX          (30)

// current config: 1 jiffy = 1 sec
#define AUTO_SLEEP_TIMEOUT          (15)

// TODO: app version detection
#define SAVE_MAGIC                  (0xC0CAFE00U)

// require to be pow of 2
#define SCAN_LOG_SIZE               (1 << 5)

#define I2C_CAPACITY                (256 * sizeof(uint8_t))

#define I2C_CAL_SIZE                (64 * sizeof(uint8_t))
#define I2C_DATA_SIZE               (I2C_CAPACITY - I2C_CAL_SIZE)

#define I2C_CAL_ADDR                (I2C_DATA_ADDR + I2C_DATA_SIZE)
#define I2C_DATA_ADDR               (0)

// WARN: keep this in sync with app_lcd.h
enum beep_mode {
    BEEP_OFF = 0,
    BEEP_ON,
};

// WARN: keep this in sync with app_lcd.h
enum scan_mode {
    SCAN_BODY = 0,
    SCAN_SURFACE,
    NUM_SCAN_MODES,
};

enum temp_unit {
    TUNIT_C = 0,
    TUNIT_F,
    NUM_TEMP_UNITS,
};

typedef struct temp_thres {
    int16_t underflow;
    int16_t overflow;
} temp_thres_t;

typedef struct scan_log {
    uint8_t  write_idx; // NOTE: defined as 'next idx to write'
    uint8_t  last_write;
    int16_t  data[SCAN_LOG_SIZE];
} scan_log_t;

typedef struct app_cfg {
    int16_t body_cal_tweak; // least digit is float .1
    int16_t body_alarm_C;
    uint8_t temp_unit;
    uint8_t beep_on;

    uint8_t sleep_jiffies;
} app_cfg_t;

typedef struct app_save {
    uint32_t   magic;
    scan_log_t scan_log[NUM_SCAN_MODES];
    app_cfg_t  cfg;
} app_save_t;

//
// variables that required for runtime
//
typedef struct app_runtime {
    uint8_t    scan_mode;
    uint8_t    scan_mode_last;
    uint8_t    scan_show;
    uint8_t    scan_burst;
    int16_t    scan_result[NUM_SCAN_MODES];
    uint8_t    read_idx[NUM_SCAN_MODES];
    uint8_t    battery_low;

    app_save_t save;
} app_runtime_t;

extern app_runtime_t  g_runtime;
extern app_runtime_t *g_rt;
extern app_save_t    *g_save;
extern app_cfg_t     *g_cfg;
extern scan_log_t    *g_scan_log;
extern temp_thres_t   g_temp_thres[];
extern CalData_t     *g_cal;

static __always_inline void app_runtime_readidx_rebase(app_runtime_t *rt)
{
    for (int i = 0; i < NUM_SCAN_MODES; i++)
        rt->read_idx[i] = rt->save.scan_log[i].write_idx;
}

static __always_inline int16_t lcd_show_C2F(int16_t C)
{
    return (C * 18 / 10 + 320);
}

static __always_inline int16_t C2F_by_setting(int16_t C)
{
    if (g_cfg->temp_unit == TUNIT_F)
        return lcd_show_C2F(C);

    return C;
}

static __always_inline int lcd_show_idx(int i)
{
    return i + 1;
}

static __always_inline int is_temp_valid(temp_thres_t *thrs, int16_t t)
{
    if (t < thrs->underflow || t > thrs->overflow)
        return 0;

    return 1;
}

static __always_inline void scan_log_idx_increase(uint8_t *idx)
{
    *idx = (*idx + 1) & (SCAN_LOG_SIZE - 1);
}

static __always_inline void scan_log_idx_decrease(uint8_t *idx)
{
    *idx = (*idx - 1) & (SCAN_LOG_SIZE - 1);
}

static __always_inline int16_t scan_log_last_written(scan_log_t *log)
{
    return log->data[log->last_write];
}

void app_runtime_init(app_runtime_t *rt);

int app_save_verify(app_save_t *save);
void app_save_reset(app_save_t *save);

int app_save_i2c_load(app_save_t *save);
int app_save_i2c_store(app_save_t *save);
int app_save_i2c_verify_with(app_save_t *save);

uint8_t scan_mode_runtime_update(void);
int16_t scan_log_read(scan_log_t *log, uint8_t idx);
void scan_log_write(scan_log_t *log, int16_t data);

#endif /* __APP_DATA_H__ */