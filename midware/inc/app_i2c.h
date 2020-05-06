/* i2c eeprom api header file */
#ifndef __APP_I2C_H__
#define __APP_I2C_H__

#include "base_types.h"

extern void sys_i2c_init();

extern boolean_t app_i2c_stop();

extern boolean_t app_i2c_start();

extern boolean_t app_i2c_write_data(uint8_t addr, const uint8_t* pdata, uint16_t len);

extern boolean_t app_i2c_read_data(uint8_t addr, uint8_t* pdata, uint16_t len);

extern void app_i2c_dump();


#endif /* end __APP_I2C_H__*/

