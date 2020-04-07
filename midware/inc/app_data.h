#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <stdint.h>

#define BODY_TEMP_LOW_THRES         (300)
#define BODY_TEMP_HI_THRES          (BODY_HI_THRESH_MAX)

#define SURFACE_TEMP_LOW_THRES      (0)
#define SURFACE_TEMP_HI_THRES       (1000)

#define BODY_HI_THRESH_MIN          (372)
#define BODY_HI_THRESH_MAX          (386)

#define CAL_OFFSET_MIN              (-32766)
#define CAL_OFFSET_MAX              (32766)

// TODO: app version detection
#define SAVE_MAGIC                  (0xCC01)

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
    int16_t low;
    int16_t high;
} temp_thres_t;

typedef struct scan_log {
    uint8_t  write_idx; // NOTE: defined as 'next idx to write'
    uint8_t  last_write;
    int16_t  data[SCAN_LOG_SIZE];
} scan_log_t;

typedef struct app_cfg {
    temp_thres_t temp_thres[NUM_SCAN_MODES];
    int16_t      cal_offset;     // last digit is consider as float .1
    uint8_t      temp_unit;
    uint8_t      beep_on;
} app_cfg_t;

typedef struct app_save {
    uint16_t   magic;
    app_cfg_t  cfg;
    scan_log_t scan_log[NUM_SCAN_MODES];
} app_save_t;

//
// variables that required for runtime
//
typedef struct app_runtime {
    int16_t    scan_result;
    uint8_t    scan_done;
    uint8_t    scan_mode;
    uint8_t    battery_low;
    uint8_t    read_idx[NUM_SCAN_MODES];
    app_save_t save;
} app_runtime_t;

extern app_runtime_t  g_runtime;
extern app_runtime_t *g_rt;
extern app_save_t    *g_save;
extern app_cfg_t     *g_cfg;
extern scan_log_t    *g_scan_log;

static inline int16_t lcd_show_C2F(int16_t C)
{
    return (C * 18 / 10 + 320);
}

static inline int lcd_show_idx(int i)
{
    return i + 1;
}

static inline int is_temp_in_range(temp_thres_t *thrs, int16_t t)
{
    if (t < thrs->low || t > thrs->high)
        return 0;

    return 1;
}

static inline void scan_log_idx_increase(uint8_t *idx)
{
    *idx = (*idx + 1) & (SCAN_LOG_SIZE - 1);
}

static inline void scan_log_idx_decrease(uint8_t *idx)
{
    *idx = (*idx - 1) & (SCAN_LOG_SIZE - 1);
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