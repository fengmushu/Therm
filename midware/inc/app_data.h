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

typedef struct app_cfg {
    int16_t body_cal_tweak; // least digit is float .1
    int16_t body_alarm_C;
    uint8_t temp_unit;
    uint8_t beep_on;

    uint8_t sleep_jiffies;
} app_cfg_t;

//
// variables that required for runtime
//
typedef struct app_runtime {
    uint8_t    scan_mode;
    uint8_t    scan_mode_last;
    uint8_t    scan_show;
    uint8_t    scan_done;
    int16_t    scan_result[NUM_SCAN_MODES];
    uint8_t    read_idx[NUM_SCAN_MODES];
    uint8_t    battery_low;

} app_runtime_t;

extern app_runtime_t  g_runtime;
extern app_runtime_t *g_rt;
extern app_cfg_t     *g_cfg;
extern temp_thres_t   g_temp_thres[];
extern CalData_t     *g_cal;

void app_runtime_init(app_runtime_t *rt);
uint8_t scan_mode_runtime_update(void);

#endif /* __APP_DATA_H__ */