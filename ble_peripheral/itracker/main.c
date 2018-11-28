/**
 * Copyright (c) 2014 - 2017, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup ble_sdk_uart_over_ble_main main.c
 * @{
 * @ingroup  ble_sdk_app_nus_eval
 * @brief    UART over BLE application main file.
 *
 * This file contains the source code for a sample application that uses the Nordic UART service.
 * This application uses the @ref srvlib_conn_params module.
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "nordic_common.h"
#include "nrf.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_freertos.h"
#include "nrf_drv_clock.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "app_timer.h"
#include "app_uart.h"
#include "app_util_platform.h"

#include "bsp_itracker.h"
#include "bsp_btn_ble.h"

#include "crc32.h"
#include "nrf_crypto_init.h"
#include "nrf_crypto_hash.h"
#include "nrf_crypto_ecdsa.h"
#include "micro_ecc_backend_ecc.h"
#include "nrf_crypto_ecc.h"
#include "nrf_power.h"
#include "nrf_nvmc.h"
#include "nrf_dfu_flash.h"
#include "nrf_fstorage.h"

#include "bleMain.h"
#include "sensorMain.h"
#include "dfuTask.h"
#include "MqttFreeRTOSTcp.h"

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
extern char GSM_RSP[1600];
__ALIGN(4) extern const uint8_t pk[64];
extern nrf_crypto_ecc_public_key_t              m_public_key;

/***************************FreeRTOS********************************/
/*considering the limination of ram and power,num of tasks should as little as possible */

#define DEMO_INTERVAL        15000
TimerHandle_t m_demo_timer;

GSM_RECIEVE_TYPE g_type = GSM_TYPE_CHAR;

/*----------------------------iTracker application-------------------------------*/
uint8_t gsm_started = 0;

/****************************************************
 *             External Functions
 */
extern void Gsm_Uart_Init(void);
extern void Gsm_Gpio_Init();
extern void Gsm_PowerUp();
extern void delay_ms(uint32_t ms);




/*************************** Log ********************************/
#ifdef DEBUG_WOLFSSL
#include "logging.h"

void log_print(const int logLevel, const char *const logMessage)
{
		DPRINTF(LOG_DEBUG, "%s\r\n", logMessage);
}
#endif

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
	
	#ifdef DEBUG_WOLFSSL
		wolfSSL_SetLoggingCb(log_print);
	#endif
}

/*************************** Timer ********************************/

static void demo_timeout_handler(TimerHandle_t xTimer)
{

    DPRINTF(LOG_INFO, "demo_timeout_handler\r\n");
    UNUSED_PARAMETER(xTimer);
    DPRINTF(LOG_INFO, "++++++++++++gps data++++++++++++\r\n");
    gps_data_get();
    DPRINTF(LOG_INFO, "++++++++++++sensor data++++++++++++\r\n");
    sensor_collect_timer_handle();
    //DPRINTF(LOG_INFO, "++++++++++++NBIOT test++++++++++++\r\n");
    //gsm_send_test();
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
    m_demo_timer = xTimerCreate("demo",
                                DEMO_INTERVAL,
                                pdTRUE,
                                NULL,
                                demo_timeout_handler);

    /* Error checking */
    if ( (NULL == m_demo_timer))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }
}


/**@brief   Function for starting application timers.
 * @details Timers are run after the scheduler has started.
 */
void application_timers_start(void)
{
    // Start application timers, non-block
    if (pdPASS != xTimerStart(m_demo_timer, 0))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

}

/**@brief Function for placing the application in low power state while waiting for events.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}
/**@brief A function which is hooked to idle task.
 * @note Idle hook must be enabled in FreeRTOS configuration (configUSE_IDLE_HOOK).
 */
void vApplicationIdleHook( void )
{
    while(1)
    {
        //DPRINTF(LOG_DEBUG,"idle task\r\n");
        //power_manage();
    }
}

/***************************FreeRTOS********************************/

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

/**@brief Application main function.
 */

int main(void)
{
    uint32_t err_code;
    bool erase_bonds;
	  int ret;

	  // log
    log_init();

	  //clock init
	  nrf_drv_clock_init();

	  // Activate deep sleep mode.
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

		// timer init
	  //timers_init();

		// ble
    ble_stack_init();
    gap_params_init();
    gatt_init();
    services_init();
		advertising_init();
    conn_params_init();
	  peer_manager_init();

		// dfu
    dfu_settings_init();
		nrf_crypto_init();
		nrf_crypto_ecc_public_key_from_raw(&g_nrf_crypto_ecc_secp256r1_curve_info, &m_public_key,pk, sizeof(pk));

		// sensors
	  //sensors_init();

		// init gsm
	  Gsm_Uart_Init();
	  Gsm_Gpio_Init();
	  Gsm_PowerUp();

    // Create a FreeRTOS task for the BLE stack. The task will run advertising_start() before entering its loop.
    nrf_sdh_freertos_init(advertising_start, NULL);
		
		// dfu task init
		//BaseType_t xReturned = xTaskCreate(dfu_task, "dfu", 512, NULL, 1, NULL);

		// mqtt task init
		MQTT_init();


    // Start FreeRTOS scheduler.
    vTaskStartScheduler();

    for (;;)
    {
        APP_ERROR_HANDLER(NRF_ERROR_FORBIDDEN);
    }
}

char * float2str(float val, int precision, char *buf)
{
    char *cur, *end;

    sprintf(buf, "%.6f", val);
    if (precision < 6) {
        cur = buf + strlen(buf) - 1;
        end = cur - 6 + precision;
        while ((cur > end) && (*cur == '0')) {
            *cur = '\0';
            cur--;
        }
    }

    return buf;
}

char * double2str(double val, int precision, char *buf)
{
    char *cur, *end;

    sprintf(buf, "%.6f", val);
    if (precision < 6) {
        cur = buf + strlen(buf) - 1;
        end = cur - 6 + precision;
        while ((cur > end) && (*cur == '0')) {
            *cur = '\0';
            cur--;
        }
    }

    return buf;
}

char lati_buf[128];
char longi_buf[128];
uint8_t collect_flag = 0;

//FILE *fp = NULL;
void swap(uint8_t* st,uint8_t len)
{
	uint8_t i;
	uint8_t t = 0;
  for (i = 0; i < len/2; i++)
	{
	  t = st[i];
		st[i] = st[len-1-i];
		st[len-1-i] = t;
	}
}

/**
 * @}
 */
