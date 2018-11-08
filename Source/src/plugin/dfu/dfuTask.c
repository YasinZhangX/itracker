#include "dfuTask.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "nrf_dfu_types.h"
#include "nrf_dfu_req_handler.h"
#include "nrf_fstorage.h"
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

extern GSM_RECIEVE_TYPE g_type;
extern TimerHandle_t m_demo_timer;  
extern char GSM_RSP[1600];
extern unsigned char dfu_cc_packet[136];
//dfu task
nrf_dfu_settings_t s_dfu_settings;
extern uint8_t m_dfu_settings_buffer[BOOTLOADER_SETTINGS_PAGE_SIZE];
__ALIGN(4) extern const uint8_t pk[64];

//signature 
extern nrf_crypto_ecdsa_secp256r1_signature_t       m_signature;
extern nrf_crypto_hash_sha256_digest_t              m_init_packet_hash;
static uint8_t m_fw_veision = 0;
static uint8_t m_hw_version = 0;

extern nrf_crypto_ecc_public_key_t              m_public_key;
extern void generate_pb(void);
extern nrf_dfu_result_t signature_check(void);
extern uint32_t firmware_copy(uint32_t dst_addr,uint8_t *src_addr,uint32_t size);
uint16_t eraser_flag = 0;

void check_answer()
{
		memset(GSM_RSP,0,1600);
		Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 4,true);
		memset(GSM_RSP,0,1600);
		Gsm_print("AT+QIRD=11,1500");
		Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 8,true);
}

void dfu_task(void * pvParameter)
{
		char len[10] = {0};
		static nrf_crypto_hash_sha256_digest_t m_result_hash;
		static int bin_flag = 0;
		static int dat_flag = -1;
		size_t hash_len = NRF_CRYPTO_HASH_SIZE_SHA256;
		size_t sign_len = NRF_CRYPTO_ECDSA_SECP256R1_SIGNATURE_SIZE;
		uint32_t image_len = 0;
		ret_code_t err_code;
		uint32_t flash_write_len = 0;
		uint32_t str = 0x0004B800;
    int j = 0;	// begin of len in packet
		int k = 0;  //begin of len num 
		int flash_write_len_1=0;
		static int packet_num = 0;
    while (true)
    {
			  DPRINTF(LOG_INFO, "Enter dfu task\r\n");	
        check_answer();
				DPRINTF(LOG_INFO, "recieve GSM_RSP len = %d\r\n",strlen(GSM_RSP));
				DPRINTF(LOG_INFO, "recieve GSM_RSP  = %s\r\n",GSM_RSP);			
			  if(strstr(GSM_RSP,"update"))
				{  
					DPRINTF(LOG_INFO, "update begin and stop other task\r\n");
					//stop data collect task
					xTimerStop(m_demo_timer,0);
					ret_code_t ret;
          ret = nrf_sdh_disable_request();
          delay_ms(2000);					
		      DPRINTF(LOG_INFO, "ret = %d\r\n",ret);
					
					s_dfu_settings.bank_1.bank_code = NRF_DFU_BANK_VALID_APP;
					s_dfu_settings.crc = 0x01020304;
					//write back to flash
				  nrf_nvmc_page_erase(0x0007F000);
					delay_ms(2000);
					nrf_nvmc_write_bytes(0x0007F000,(uint8_t *)(&s_dfu_settings),sizeof(s_dfu_settings));
					delay_ms(2000);
					
					Gsm_print("AT+QISEND=11,22");
					memset(GSM_RSP,0,1600);
			    Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT*4,true);	
					Gsm_print("please send dat file\r\n");
					memset(GSM_RSP,0,1600);
			    Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT*4,true);
					dat_flag = 0;
					g_type = GSM_TYPE_FILE;
        }
				//check signature with public key ,135 means dat len
				else if(strstr(GSM_RSP,"135") && dat_flag == 0)
				{
					DPRINTF(LOG_INFO, "begin to recieve signature\r\n");
					//store hash and signature from init packet and skip 13 u8 because (\r\n+QIRD: 135\r\n)
					memcpy(m_signature,&GSM_RSP[71+14],64);
					memcpy(&m_fw_veision,&GSM_RSP[10+14],1);
					memcpy(&m_hw_version,&GSM_RSP[12+14],1);	
          DPRINTF(LOG_INFO, "begin to recieve init packet:\r\n"); 			
          DPRINTF(LOG_INFO, "m_fw_veision = %d:\r\n",m_fw_veision); 
          DPRINTF(LOG_INFO, "m_hw_version = %d:\r\n",m_hw_version); 					
					dat_flag =-1 ;
					
					memcpy(dfu_cc_packet,&GSM_RSP[14],135);
          generate_pb();
					err_code = signature_check();
					if(err_code == NRF_DFU_RES_CODE_SUCCESS)
					{
							Gsm_print("AT+QISEND=11,22");
			        Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);	
							Gsm_print("please send bin file\r\n");
			        Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);
					}
					else
					{
					    dat_flag = -1;
							Gsm_print("AT+QISEND=11,23");
						  memset(GSM_RSP,0,1600);
			        Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT,true);	
							Gsm_print("verify fail and reset\r\n");
							memset(GSM_RSP,0,1600);
			        Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT,true);
						  NVIC_SystemReset();
					}
				}
				//begin recieve packet may above 10 packets
				else if(strstr(GSM_RSP,"bin"))
				{
					  bin_flag = 1;
					  memset(GSM_RSP,0,1600);
				}
				else if(bin_flag == 1)
				{
					  while(GSM_RSP[9]!='0')    //(\r\n+QIRD: xx\r\n) real packet
						{
							  DPRINTF(LOG_INFO, "begin to recieve bin\r\n");
						    //calc the single packet length
							  for(j=9;GSM_RSP[j]!='\r';j++)
							  {
								    len[k++] = GSM_RSP[j];
								}
								//skip to real bin begin
								j = j+2;
							  image_len += atoi(len);
								flash_write_len = atoi(len);
								//store to flash
								if (flash_write_len>0 && flash_write_len <4)
								{
								   flash_write_len = 4;
								}
								else if(flash_write_len>=4)
								{
								   flash_write_len_1 = flash_write_len % 4;
									 if(flash_write_len_1!=0)
								   {
								      flash_write_len = (flash_write_len/4 +1) * 4;
							     }	 
									  
								}
							  firmware_copy(str,&GSM_RSP[j],flash_write_len);
								str+=flash_write_len;
								packet_num++;
								DPRINTF(LOG_INFO, "dfu have recieved %d packet\r\n",packet_num);
								memset(len,0,10);
								k = 0;
								j = 0;
								delay_ms(100);
							  //recieve next
							  Gsm_print("AT+QIRD=11,1500");
                memset(GSM_RSP,0,1600);
                Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 8,true);
                if(GSM_RSP[9]=='0')
								{
								   DPRINTF(LOG_INFO, "packet recieve finish and reset\r\n");
									 NVIC_SystemReset();
								}									
						}
				}
				else
				{
			     	//DPRINTF(LOG_INFO, "nothing to do!\r\n");
				}
    }
}

void dfu_settings_init(void)
{
    memset(&s_dfu_settings, 0x00, sizeof(nrf_dfu_settings_t));
	  // Copy the DFU settings out of flash and into a buffer in RAM.
    memcpy((void*)&s_dfu_settings, m_dfu_settings_buffer, sizeof(nrf_dfu_settings_t));
	  s_dfu_settings.settings_version = 0x01;
	  s_dfu_settings.bootloader_version = 0x01;
	  s_dfu_settings.app_version = 0x01;
	  s_dfu_settings.bank_layout = 0;
	  s_dfu_settings.bank_1.image_size = 80*1024;
	  s_dfu_settings.progress.update_start_address = 0x0004B800;
	  s_dfu_settings.write_offset = 0;
} 