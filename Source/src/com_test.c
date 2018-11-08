
#include "nrf_delay.h"
#include "nrf_drv_rtc.h"
#include "nrf_rtc.h"

#include "bsp_itracker.h"

#define COM_UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define COM_UART_RX_BUF_SIZE 64                      /**< UART RX buffer size. */


void com_uart_event_handle(app_uart_evt_t * p_event)
{
    static uint8_t index = 0;
    uint32_t       err_code;
		uint8_t rx_data;
    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
   	     	
						app_uart_get(&rx_data);
						SEGGER_RTT_printf(0, "%c", rx_data);	
            break;

        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}

void Com_Uart_Init(void)
{
	 uint32_t err_code;
	 app_uart_close();
	 const app_uart_comm_params_t comm_params =
	 {
	 			28,
	 			29,
	 			0,
	 			0,
	 			APP_UART_FLOW_CONTROL_DISABLED,
	 			false,
	 			UART_BAUDRATE_BAUDRATE_Baud115200
	 };
	 
	 APP_UART_FIFO_INIT(&comm_params,
	 					COM_UART_TX_BUF_SIZE,
	 					COM_UART_RX_BUF_SIZE,
	 					com_uart_event_handle,
	 					APP_IRQ_PRIORITY_LOW,
	 					err_code);
	 
	 APP_ERROR_CHECK(err_code);
}


int COM_UART_TxBuf(uint8_t *buffer)
{
		uint32_t err_code;
		for (uint32_t i = 0; buffer[i] != '\0'; i++)
		{
				do
				{
						err_code = app_uart_put(buffer[i]);
						if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
						{
								//NRF_LOG_ERROR("Failed receiving NUS message. Error 0x%x. ", err_code);
								APP_ERROR_CHECK(err_code);
						}
				} while (err_code == NRF_ERROR_BUSY);
		}
		return err_code;
}

void Com_Test_1()
{
 	uint8_t c = '1';
	while(c<='9')
	{
		 app_uart_put(c++);
	   nrf_delay_ms(1000);			
	}
}
void Com_Test_2()
{
 	uint8_t c = '1';
	while(c<='9')
	{
		if(c>='9')
		{
		     c='1';	
		}
		else
		{
		     app_uart_put(c++);
	       nrf_delay_ms(1000);			
		}
	}
}