
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "nrf_drv_twi.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "bsp_itracker.h"
#include "rak_i2c_sensor.h"
#include "sensor_opt3001.h"

#define OPT3001_ADDR          0x44


uint32_t opt3001_twi_init(void)
{
    uint32_t err_code;
    
    const nrf_drv_twi_config_t twi_config = {
       .scl                = OPT3001_TWI_SCL_PIN,
       .sda                = OPT3001_TWI_SDA_PIN,
       .frequency          = NRF_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGHEST
    };
		
    rak_i2c_deinit();
    err_code = rak_i2c_init(&twi_config);
    if(err_code != NRF_SUCCESS)
	  {
		    return err_code;
	  }
	
	  return NRF_SUCCESS;
}

/**************************************************************************************************
 * @fn          sensorReadReg
 *
 * @brief       This function implements the I2C protocol to read from a sensor. The sensor must
 *              be selected before this routine is called.
 *
 * @param       addr - which register to read
 * @param       pBuf - pointer to buffer to place data
 * @param       nBytes - numbver of bytes to read
 *
 * @return      TRUE if the required number of bytes are received
 **************************************************************************************************/
bool sensorReadReg(uint8_t addr, uint8_t *pBuf, uint8_t nBytes)
{
	if(rak_i2c_read(OPT3001_ADDR, addr,pBuf,nBytes) == NRF_SUCCESS)
	{
			return true;
	}
  else 
	{
			return false;
	}
}

/**************************************************************************************************
* @fn          sensorWriteReg
* @brief       This function implements the I2C protocol to write to a sensor. he sensor must
*              be selected before this routine is called.
*
* @param       addr - which register to write
* @param       pBuf - pointer to buffer containing data to be written
* @param       nBytes - number of bytes to write
*
* @return      TRUE if successful write
*/
bool sensorWriteReg(uint8_t addr, uint8_t *pBuf, uint8_t nBytes)
{
	if(rak_i2c_write(OPT3001_ADDR, addr, pBuf, nBytes) == NRF_SUCCESS)
	{
			return true;
	}
	else 
	{
			return false;
	}
}

int opt3001_init(void)
{
    int ret;
		uint16_t manufacture_id=0, device_id=0;
	
		sensorOpt3001Enable(1);

		ret = opt3001_get_id(&manufacture_id, &device_id);
		if(ret)
		{
				DPRINTF(LOG_INFO, "opt3001:manufacture_id=%x device_id=%x\r\n", manufacture_id, device_id);
		}
		else 
		{
				return -1;
		}
		return 0;
}

int get_opt3001_data(float *light_data)
{
		int ret;
		int count =1;
		uint16_t light_raw_data=0;
		//float light_data=0;
		while(count--)
		{
			ret = sensorOpt3001Read(&light_raw_data);
			//DPRINTF(LOG_INFO, "light=%d %d\r\n", light_raw_data, ret);
			*light_data = sensorOpt3001Convert(light_raw_data);
			//nrf_delay_ms(1000);
			//vTaskDelay(100);
		}
}

void opt3001_deinit()
{
		rak_i2c_deinit();
}