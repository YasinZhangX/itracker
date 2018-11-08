#include "sensorMain.h"
#include "bsp_itracker.h"

#define  POST_FORMAT 				\
				"{\r\n"							\
				"\"temp\" : %.2f,\r\n"				\
				"\"humi\" : %.2f,\r\n"				\
				"\"press\" : %.2f,\r\n"				\
				"\"light\" : %.2f,\r\n"				\
				"\"X\" : %d,\r\n"				\
				"\"Y\" : %d,\r\n"		\
				"\"Z\" : %d,\r\n"			\
				"\"m_x\" : %.2f,\r\n"				\
				"\"m_y\" : %.2f,\r\n"\
				"\"m_z\" : %.2f\r\n"\
				"}\r\n"

tracker_data_t tracker_data;
char post_data[1024]={0};

void sensors_init()
{
		int ret;
#ifdef BEM280_TEST
		ret = bme280_spi_init();
		if(ret != NRF_SUCCESS) {
				DPRINTF(LOG_ERROR, "bme280_spi_init fail %d\r\n", ret);
		} else {
				ret = _bme280_init();
				if(ret < 0) {
						DPRINTF(LOG_ERROR, "lis3dh_init fail\r\n");
				}
		}
#endif
#ifdef LIS3DH_TEST
		ret = lis3dh_twi_init();
		if(ret != NRF_SUCCESS) {
				DPRINTF(LOG_ERROR, "lis3dh_twi_init fail %d\r\n", ret);
		} else {
				ret = lis3dh_init();
				if(ret < 0) {
						DPRINTF(LOG_ERROR, "lis3dh_init fail\r\n");
				}
		}
#endif
#ifdef LIS2MDL_TEST
		ret = lis2mdl_twi_init();
		if(ret < 0) {
				DPRINTF(LOG_ERROR, "lis2mdl_twi_init fail %d\r\n", ret);
		} else {
				ret = lis2mdl_init();
				if(ret < 0) {
						DPRINTF(LOG_ERROR, "lis2mdl_init fail\r\n");
				}
		}
#endif
#ifdef OPT3001_TEST
		ret = opt3001_twi_init();
		if(ret < 0) {
				DPRINTF(LOG_ERROR, "opt3001_twi_init fail %d\r\n", ret);
		} else {
				ret = opt3001_init();
				if(ret < 0) {
						DPRINTF(LOG_ERROR, "opt3001_init fail\r\n");
				}
		}
#endif
}

void start_collect_data()
{
		int ret;
#ifdef BEM280_TEST
		get_bme280_data(&tracker_data.temp_value, &tracker_data.humidity_value, &tracker_data.gas_value);
#endif
#ifdef LIS3DH_TEST
		ret = lis3dh_twi_init();
		if(ret != NRF_SUCCESS) {
				DPRINTF(LOG_ERROR, "lis3dh_twi_init fail %d\r\n", ret);
		} else {
				get_lis3dh_data(&tracker_data.x, &tracker_data.y, &tracker_data.z);		
		}
#endif
#ifdef LIS2MDL_TEST
		ret = lis2mdl_twi_init();
		if(ret != NRF_SUCCESS) {
				DPRINTF(LOG_ERROR, "lis2mdl_twi_init fail %d\r\n", ret);
		} else {
				get_lis2mdl_data(&tracker_data.magnetic_x, &tracker_data.magnetic_y, &tracker_data.magnetic_z);
		}
#endif
#ifdef OPT3001_TEST
		ret = opt3001_twi_init();
		if(ret != NRF_SUCCESS) {
				DPRINTF(LOG_ERROR, "opt3001_twi_init fail %d\r\n", ret);
		} else {
				get_opt3001_data(&tracker_data.light_value);		
		}
#endif
}

void sensor_collect_timer_handle()
{
		int ret;
		static uint32_t swi_cnt=0;
	
		start_collect_data();	

		sprintf(post_data, POST_FORMAT, tracker_data.temp_value,
																		tracker_data.humidity_value,
																		tracker_data.gas_value,
																		/*tracker_data.barometer_value,*/
																		tracker_data.light_value,
																		//tracker_data.latitude,
																		//tracker_data.longitude,
																		tracker_data.x,
																		tracker_data.y,
																		tracker_data.z,
																		tracker_data.magnetic_x,
																		tracker_data.magnetic_y,
																		tracker_data.magnetic_z
																		);
		DPRINTF(LOG_INFO, "%s", post_data);
}