#ifndef SENSORMAIN_H
#define SENSORMAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#define LIS3DH_TEST
#define LIS2MDL_TEST
#define BEM280_TEST
#define OPT3001_TEST

#include "rak_spi_sensor.h"
#include "rak_i2c_sensor.h"
#include "bsp_itracker.h"

extern tracker_data_t tracker_data;

/**
 *  Public Functions
 */
void sensors_init(void);
void start_collect_data(void);
void sensor_collect_timer_handle(void);

#ifdef __cplusplus
}
#endif
#endif /* SENSORMAIN_H */