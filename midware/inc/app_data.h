#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#define BODY_HI_THRESH_MIN          (372)
#define BODY_HI_THRESH_MAX          (386)

#define CAL_OFFSET_MIN              (-32766)
#define CAL_OFFSET_MAX              (32766)

// WARN: keep this in sync with app_lcd.h
enum {
    BEEP_OFF = 0,
    BEEP_ON,
};

typedef struct app_cfg {
    uint16_t body_hi_temp_C;
    int16_t  cal_offset; // ??
    uint8_t  temp_unit;
    uint8_t  beep_on;
} app_cfg_t;

typedef struct app_save {
    uint8_t        calibrated; // this 
    struct app_cfg cfg;
} app_save_t;

typedef struct app_runtime {
    uint8_t  scan_mode;

} app_runtime_t;

extern app_save_t g_save;

int app_data_load(app_save_t *save);
int app_data_reset(app_save_t *save);

#endif /* __APP_DATA_H__ */