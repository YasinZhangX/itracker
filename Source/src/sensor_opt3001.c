/******************************************************************************
*  Filename:       sensor_opt3001.c
*  Revised:        $Date: 2015-01-31 05:01:21 -0800 (Sat, 31 Jan 2015) $
*  Revision:       $Revision: 14988 $
*
*  Description:    Driver for the Opt3001 light sensor.
*
*  Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
*
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/


/* -----------------------------------------------------------------------------
*                                          Includes
* ------------------------------------------------------------------------------
*/

#include "sensor_opt3001.h"
#include "math.h"
/* -----------------------------------------------------------------------------
*                                           Constants
* ------------------------------------------------------------------------------
*/

/* Slave address */
#define OPT3001_I2C_ADDRESS             0x45

/* Register addresses */
#define REG_RESULT                      0x00
#define REG_CONFIGURATION               0x01
#define REG_LOW_LIMIT                   0x02
#define REG_HIGH_LIMIT                  0x03

#define REG_MANUFACTURER_ID             0x7E
#define REG_DEVICE_ID                   0x7F

/* Register values */
#define MANUFACTURER_ID                 0x5449  // TI
#define DEVICE_ID                       0x3001  // Opt 3001
#define CONFIG_RESET                    0xC810                   
#define CONFIG_TEST                     0xCC10
#define CONFIG_ENABLE                   0x10CC // 0xCC10                  
#define CONFIG_DISABLE                  0x108C // 0xC810

/* Bit values */
#define DATA_RDY_BIT                    0x0080  // Data ready

/* Register length */
#define REGISTER_LENGTH                 2

/* Sensor data size */
#define DATA_LENGTH                     2

/* Conversion macros */
#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

/* -----------------------------------------------------------------------------
*                                           Local Variables
* ------------------------------------------------------------------------------
*/

/* -----------------------------------------------------------------------------
*                                           Public functions
* ------------------------------------------------------------------------------
*/


/*******************************************************************************
 * @fn          sensorOpt3001Init
 *
 * @brief       Initialise the temperature sensor driver
 *
 * @return      none
 ******************************************************************************/
void sensorOpt3001Init(void)
{
  sensorOpt3001Enable(false);
}


/*******************************************************************************
 * @fn          sensorOpt3001Enable
 *
 * @brief       Turn the sensor on/off
 *
 * @return      none
 ******************************************************************************/
void sensorOpt3001Enable(uint8_t enable)
{
  uint16_t val;
  
  if (enable)
    val = CONFIG_ENABLE;
  else
    val = CONFIG_DISABLE;
  
  sensorWriteReg(REG_CONFIGURATION, (uint8_t *)&val, REGISTER_LENGTH);
}


/*******************************************************************************
 * @fn          sensorOpt3001Read
 *
 * @brief       Read the result register
 *
 * @param       Buffer to store data in
 *
 * @return      TRUE if valid data
 ******************************************************************************/
int sensorOpt3001Read(uint16_t *rawData)
{
  int success;
  uint16_t val;
  
  success = sensorReadReg(REG_CONFIGURATION, (uint8_t *)&val, REGISTER_LENGTH);

  if (success)
    success = (val & DATA_RDY_BIT) == DATA_RDY_BIT;

  if (success)
    success = sensorReadReg(REG_RESULT, (uint8_t*)&val, DATA_LENGTH);
  
  if (success)
  {
    // Swap bytes
    *rawData = (val << 8) | (val>>8 &0xFF);
  } 
  else
  {
//    sensorSetErrorData((uint8_t*)rawData, DATA_LENGTH);
  }
  
  return success;
}


/*******************************************************************************
 * @fn          sensorOpt3001Test
 *
 * @brief       Run a sensor self-test
 *
 * @return      TRUE if passed, FALSE if failed
 ******************************************************************************/

int sensorOpt3001Test(void)
{
  uint16_t val;
	int success;

  // Check manufacturer ID
  success = sensorReadReg(REG_MANUFACTURER_ID, (uint8_t *)&val, REGISTER_LENGTH);
	if(success)
	{
			val = (LO_UINT16(val) << 8) | HI_UINT16(val);
			if(val != MANUFACTURER_ID)
			{
					return false;
			}
	}
	else 
	{
			return false;
	}
  // Check device ID
	success = sensorReadReg(REG_DEVICE_ID, (uint8_t *)&val, REGISTER_LENGTH);
	if(success)
	{
			val = (LO_UINT16(val) << 8) | HI_UINT16(val);
			if(val != DEVICE_ID)
			{
					return false;
			}				
	}
	else 
	{
			return false;
	}
	
  return success;
}

int opt3001_get_id(uint16_t *manufacture_id, uint16_t *device_id)
{
  uint16_t val;
	int success;

  // Check manufacturer ID
  success = sensorReadReg(REG_MANUFACTURER_ID, (uint8_t *)&val, REGISTER_LENGTH);
	if(success)
	{
			val = (LO_UINT16(val) << 8) | HI_UINT16(val);
			if(val != MANUFACTURER_ID)
			{
					return false;
			}
			else 
			{
					*manufacture_id = val;
			}
	}
	else 
	{
			return false;
	}
  // Check device ID
	success = sensorReadReg(REG_DEVICE_ID, (uint8_t *)&val, REGISTER_LENGTH);
	if(success)
	{
			val = (LO_UINT16(val) << 8) | HI_UINT16(val);
			if(val != DEVICE_ID)
			{
					return false;
			}		
			else 
			{
					*device_id = val;
			}			
	}
	else 
	{
			return false;
	}
	
  return success;
}

/*******************************************************************************
 * @fn          sensorOpt3001Convert
 *
 * @brief       Convert raw data to object and ambience tempertaure
 *
 * @param       data - raw data from sensor
 *
 * @param       lux - converted value (lux)
 *
 * @return      none
 ******************************************************************************/
float sensorOpt3001Convert(uint16_t rawData)
{
  uint16_t e, m;

  m = rawData & 0x0FFF;
  e = (rawData & 0xF000) >> 12;
  
  return m * (0.01 * exp2(e));
}

/*******************************************************************************
*******************************************************************************/

