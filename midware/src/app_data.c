#include <stdint.h>

#include "app.h"
#include "app_lcd.h"
#include "app_data.h"

app_save_t g_save;

static app_cfg_t default_cfg = {
	.body_hi_temp_C = 380,
	.cal_offset 	= 0,
	.temp_unit 	= Celsius,
	.beep_on 	= BEEP_ON,
};

void app_save_reset(app_save_t *save)
{
	memset(save, 0x00, sizeof(*save));
	memcpy(&save->cfg, &default_cfg, sizeof(save->cfg));

	// REVIEW: REMOVE ME
	save->calibrated = 1;
}

// TODO: cal, data, cfg load/save/add/delete/mod/query/struct_management here

