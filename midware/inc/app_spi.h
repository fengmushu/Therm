#ifndef __APP_SPI_H__
#define __APP_SPI_H__

#include "gpio.h"
#include "base_types.h"

extern void sys_spi_init(void);
extern void plat_spi_start(void);
extern void plat_spi_stop(void);
extern void plat_spi_deinit(void);
extern void plat_spi_xmit(const uint8_t *data, uint16_t len);

#endif //__APP_SPI_H__
