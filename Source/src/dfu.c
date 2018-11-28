#include <stdbool.h>
#include "nrf_dfu_types.h"
#include "nrf_dfu_settings.h"
#include "nrf_dfu_utils.h"
#include "nrf_dfu_flash.h"
#include "nrf_bootloader_info.h"
#include "pb.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "dfu-cc.pb.h"
#include "crc32.h"
#include "nrf_crypto.h"
#include "nrf_assert.h"
#include "nrf_dfu_validation.h"
#include "nrf_dfu_ver_validation.h"
#include "nrf_crypto_init.h"
#include "nrf_crypto_hash.h"
#include "nrf_crypto_ecdsa.h"
#include "micro_ecc_backend_ecc.h"
#include "nrf_crypto_ecc.h"
#include "nrf_fstorage.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "bsp_itracker.h"
#include "nrf_dfu_types.h"
#include "nrf_dfu_settings.h"
#include <stddef.h>
#include <string.h>
#include "app_error.h"
#include "nrf_dfu_flash.h"
#include "nrf_soc.h"
#include "crc32.h"
#include "nrf_nvmc.h"

#define DFU_SETTINGS_INIT_COMMAND_OFFSET        offsetof(nrf_dfu_settings_t, init_command)          //<! Offset in the settings struct where the InitCommand is located.

#define NRF_LOG_MODULE_NAME nrf_dfu_settings
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();


/**@brief   This variable reserves a page in flash for bootloader settings
 *          to ensure the linker doesn't place any code or variables at this location.
 */
#if defined (__CC_ARM )

    uint8_t m_dfu_settings_buffer[BOOTLOADER_SETTINGS_PAGE_SIZE]
        __attribute__((at(BOOTLOADER_SETTINGS_ADDRESS)))
        __attribute__((used));

#elif defined ( __GNUC__ ) || defined ( __SES_ARM )

    uint8_t m_dfu_settings_buffer[BOOTLOADER_SETTINGS_PAGE_SIZE]
        __attribute__((section(".bootloader_settings_page")))
        __attribute__((used));

#elif defined ( __ICCARM__ )

    __no_init __root uint8_t m_dfu_settings_buffer[BOOTLOADER_SETTINGS_PAGE_SIZE]
        @ BOOTLOADER_SETTINGS_ADDRESS;

#else

    #error Not a valid compiler/linker for m_dfu_settings placement.

#endif // Compiler specific

#ifndef BL_SETTINGS_ACCESS_ONLY
#if defined(NRF52_SERIES)

/**@brief   This variable reserves a page in flash for MBR parameters
 *          to ensure the linker doesn't place any code or variables at this location.
 */
#if defined ( __CC_ARM )

    uint8_t m_mbr_params_page[NRF_MBR_PARAMS_PAGE_SIZE]
        __attribute__((at(NRF_MBR_PARAMS_PAGE_ADDRESS)))
        __attribute__((used));

#elif defined ( __GNUC__ ) || defined ( __SES_ARM )

    uint8_t m_mbr_params_page[NRF_MBR_PARAMS_PAGE_SIZE]
        __attribute__ ((section(".mbr_params_page")));

#elif defined ( __ICCARM__ )

    __no_init uint8_t m_mbr_params_page[NRF_MBR_PARAMS_PAGE_SIZE]
        @ NRF_MBR_PARAMS_PAGE_ADDRESS;

#else

    #error Not a valid compiler/linker for m_mbr_params_page placement.

#endif // Compiler specific


/**@brief   This variable has the linker write the MBR parameters page address to the
 *          UICR register. This value will be written in the HEX file and thus to the
 *          UICR when the bootloader is flashed into the chip.
 */
#if defined ( __CC_ARM )

    uint32_t const m_uicr_mbr_params_page_address
        __attribute__((at(NRF_UICR_MBR_PARAMS_PAGE_ADDRESS))) = NRF_MBR_PARAMS_PAGE_ADDRESS;

#elif defined ( __GNUC__ ) || defined ( __SES_ARM )

    uint32_t const m_uicr_mbr_params_page_address
        __attribute__ ((section(".uicr_mbr_params_page")))
        __attribute__ ((used)) = NRF_MBR_PARAMS_PAGE_ADDRESS;

#elif defined ( __ICCARM__ )

    __root uint32_t const m_uicr_mbr_params_page_address
        @ NRF_UICR_MBR_PARAMS_PAGE_ADDRESS = NRF_MBR_PARAMS_PAGE_ADDRESS;

#else

    #error Not a valid compiler/linker for m_mbr_params_page placement.

#endif // Compiler specific
#endif // #if defined( NRF52_SERIES )
#endif // #ifndef BL_SETTINGS_ACCESS_ONLY



unsigned char dfu_cc_packet[136]={
    0x12,0x84,0x01,0x0A,0x3E,0x08,0x01,0x12,0x3A,0x08,0x01,0x10,0x33,0x1A,0x02,0xA8,
    0x01,0x20,0x00,0x28,0x00,0x30,0x00,0x38,0xA4,0xA6,0x03,0x42,0x24,0x08,0x03,0x12,
    0x20,0x47,0x85,0x28,0x86,0x4D,0xB8,0x4E,0xA7,0x6D,0xE4,0x70,0x2A,0x80,0xE9,0x97,
    0xD3,0x02,0xC2,0xBA,0x91,0xCB,0x7D,0xB5,0x27,0x39,0xEE,0xBF,0xFE,0x71,0x12,0x52,
    0x5F,0x48,0x00,0x10,0x00,0x1A,0x40,0xD2,0xAD,0xD7,0xEE,0x29,0x7D,0x45,0x5C,0x4C,
    0x69,0x39,0x72,0x42,0x77,0x55,0x74,0x81,0xE3,0xB6,0x1A,0xD2,0x80,0x2F,0x5B,0x2C,
    0x34,0x6B,0xE4,0xA7,0xFC,0x18,0x3E,0xDE,0xA9,0xD9,0x4E,0x3C,0xFE,0xFD,0xC7,0xC5,
    0x32,0xCF,0x21,0xA1,0x99,0x25,0xDC,0xE3,0x51,0x12,0x66,0xC2,0x3B,0x95,0x53,0xEF,
    0x15,0x09,0x51,0x1E,0xAA,0x40,0x75,0x00,
};

unsigned char m_hash[32] = 
{
    0x47,0x85,0x28,0x86,0x4D,0xB8,0x4E,0xA7,0x6D,0xE4,0x70,0x2A,0x80,0xE9,0x97,0xD3,
	  0x02,0xC2,0xBA,0x91,0xCB,0x7D,0xB5,0x27,0x39,0xEE,0xBF,0xFE,0x71,0x12,0x52,0x5F,
};

pb_istream_t stream;
dfu_packet_t packet = DFU_PACKET_INIT_DEFAULT;
uint8_t*           m_init_packet_data_ptr   = 0;
uint32_t           m_init_packet_data_len   = 0;

nrf_crypto_ecdsa_verify_context_t        m_verify_context = {0};
nrf_crypto_hash_context_t                m_hash_context = {0};


__ALIGN(4) extern const uint8_t pk[64];

/** @brief Value length structure holding the public key.
 *
 * @details The pk value pointed to is the public key present in dfu_public_key.c
 */
nrf_crypto_ecc_public_key_t                  m_public_key;

/** @brief Structure to hold a signature
 */
nrf_crypto_ecdsa_secp256r1_signature_t       m_signature;

/** @brief Structure to hold the hash for the init packet
 */
nrf_crypto_hash_sha256_digest_t              m_init_packet_hash;

uint16_t eraser_flag = 0;

uint32_t firmware_copy(uint32_t dst_addr,
                           uint8_t *src_addr,
                           uint32_t size)
{
   	uint32_t ret_val = NRF_SUCCESS;
    if (src_addr == NULL || dst_addr == 0)
    {
        NRF_LOG_DEBUG("No copy needed src_addr: 0x%x, dst_addr: 0x%x", src_addr, dst_addr);
        return NRF_SUCCESS;
    }

		int page = 30; //	1 page = 4096 bytes total 30 page
		uint32_t str = 0x0004B800;
		//we remain about 120k for bank1 start from 0x0004B800,you can change size below
    //first eraser 120k once
		while(page-- && eraser_flag == 0)
		{
		   nrf_nvmc_page_erase(str);
			 str+=0x1000;
			 delay_ms(100);
		}
		eraser_flag = 1;
		//second write to flash
    nrf_nvmc_write_bytes(dst_addr,src_addr,size);
		delay_ms(10);
    return ret_val;
}

void generate_pb(void)
{
    stream = pb_istream_from_buffer(dfu_cc_packet, 136);

    // Attach our callback to follow the field decoding
    stream.decoding_callback = NULL;
    // reset the variable where the init pointer and length will be stored.

    if (!pb_decode(&stream, dfu_packet_fields, &packet))
    {
        DPRINTF(LOG_INFO,"Handler: Invalid protocol buffer stream\r\n");
        return;
    }
    DPRINTF(LOG_INFO,"valid protocol buffer stream\r\n");
    return;
}

// Function to perform signature check if required.
nrf_dfu_result_t signature_check(void)
{
    ret_code_t err_code;
	  uint8_t *ptr = 0;
	  uint8_t len2 = 0;
    size_t     hash_len = NRF_CRYPTO_HASH_SIZE_SHA256;
    
    NRF_LOG_INFO("Signature required. Checking signature.");
	
	  ptr = (uint8_t *)(&(packet.signed_command.command));
	  len2 = sizeof(packet.signed_command.command);
	  DPRINTF(LOG_INFO,"ptr = %X , len2 = %d \r\n", ptr,len2);
    err_code = nrf_crypto_hash_calculate(&m_hash_context,
                                         &g_nrf_crypto_hash_sha256_info,
                                         ptr,
                                         len2,
                                         m_init_packet_hash,
                                         &hash_len);
	
    if (err_code != NRF_SUCCESS)
    {
        return NRF_DFU_RES_CODE_OPERATION_FAILED;
    }

    // calculate the signature
    NRF_LOG_INFO("Verify signature");
		//memcpy(&m_init_packet_hash,m_hash,32);
    err_code = nrf_crypto_ecdsa_verify(&m_verify_context,
                                       &m_public_key,
                                       m_init_packet_hash,
                                       hash_len,
                                       m_signature,
                                       sizeof(m_signature));
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Signature failed (err_code: 0x%x)", err_code);
        NRF_LOG_DEBUG("Signature:");
        NRF_LOG_HEXDUMP_DEBUG(m_signature, sizeof(m_signature));
        NRF_LOG_DEBUG("Hash:");
        NRF_LOG_HEXDUMP_DEBUG(m_init_packet_hash, hash_len);
        NRF_LOG_DEBUG("Public Key:");
        NRF_LOG_HEXDUMP_DEBUG(pk, sizeof(pk));
        NRF_LOG_FLUSH();

        return NRF_DFU_RES_CODE_INVALID_OBJECT;
    }

    DPRINTF(LOG_INFO,"Image verified");
    return NRF_DFU_RES_CODE_SUCCESS;
}
