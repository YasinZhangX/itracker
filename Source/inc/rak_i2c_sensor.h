#ifndef __RAK_I2C_SENSOR_H__
#define __RAK_I2C_SENSOR_H__

#include "stdint.h"
#include "nrf_drv_twi.h"

uint32_t rak_i2c_init(const nrf_drv_twi_config_t *twi_config);
uint32_t rak_i2c_write(uint8_t twi_addr, uint8_t reg, uint8_t *data, uint16_t len);
uint8_t rak_i2c_read(uint8_t twi_addr, uint8_t reg, uint8_t * data, uint16_t len);
void rak_i2c_deinit(void);

uint32_t lis3dh_twi_init(void);
int lis3dh_init(void);
void get_lis3dh_data(int *x, int *y, int *z);

uint32_t lis2mdl_twi_init(void);
int lis2mdl_init(void);
void get_lis2mdl_data(float *magnetic_x, float *magnetic_y, float *magnetic_z);

uint32_t opt3001_twi_init(void);
int opt3001_init(void);
int get_opt3001_data(float *light_data);
#endif
