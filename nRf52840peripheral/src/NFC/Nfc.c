/**
 * @file   : NFC.c
 * @author : Devendu
 * @brief  : File containing NFC functionalities
 * @date   : 22-03-2024
 *
*/
 
/***********************************INCLUDES**************************/
#include <zephyr/sys/reboot.h>
#include <nfc_t2t_lib.h>
#include <nfc/ndef/launchapp_msg.h>
#include <dk_buttons_and_leds.h>
#include "Nfc.h"
 
/************************************MACROS***************************/
#define NDEF_MSG_BUF_SIZE   256
#define NFC_FIELD_LED       DK_LED1
 
/************************************GLOBALS**************************/
/* URL: https://instagram.com */
static const uint8_t webpage_url[] ={
     'h', 't', 't', 'p', ':', '/', '/', 'a', 'n', 'a', 'n', 'd', '-', 'i', 'n','f', 'o', 'l', 'i','t','z','.','g','i','t','h','u','b','.','i','o'
};

 
static uint8_t ndef_Msg_Buf[NDEF_MSG_BUF_SIZE];
 
/**********************************FUNCTION DEFINITIONS****************/

static void nfc_callback(void *context,
                         nfc_t2t_event_t event,
                         const uint8_t *data,
                         size_t data_length)
{
    uint8_t ucidx = 0;
    ARG_UNUSED(context);
    ARG_UNUSED(data);
    ARG_UNUSED(data_length);
 
    switch (event) {
    case NFC_T2T_EVENT_FIELD_ON:
        //printk("field on event\n\r");
        dk_set_led_on(NFC_FIELD_LED);
        break;
    case NFC_T2T_EVENT_FIELD_OFF:
        //printk("field off event\n\r");
        dk_set_led_off(NFC_FIELD_LED);
        break;
    case NFC_T2T_EVENT_DATA_READ:
       
        break;
    default:
        break;
    }
 
}
/**
 * @brief  Sets up NFC Type 2 Tag (T2T) library and registers callbacks.
 *        
 *         This function initializes the NFC Type 2 Tag library and registers
 *         the callback function to handle NFC events such as field on and field off.
 *
 * @note   This function must be called before any other NFC-related functions are used.
 *
 * @return 0 if successful, otherwise a negative error code.
 */
int nfc_Setup(void)
{
    size_t len = sizeof(ndef_Msg_Buf);

    int err = nfc_t2t_setup(nfc_callback, NULL);

    if (err) 
    {
        return err;
    }

    if (nfc_Encode_Launch_App_Msg(webpage_url,
                                   sizeof(webpage_url),
                                   ndef_Msg_Buf,
                                   &len)) 
    {
        return err;
    }

    if(nfc_Set_Payload(ndef_Msg_Buf, len))
    {
        return err;
    }

    if(nfc_Start_Emulation())
    {
        return err;
    }

    return 0;
    
}
 
/**
 * @brief  Encodes a launch app message for NFC.
 *
 *         This function encodes a launch app message for NFC with the given URL.
 *
 * @param  url          The URL to be encoded in the launch app message.
 * @param  url_len      The length of the URL.
 * @param  ndef_msg_buf The buffer to store the encoded NDEF message.
 * @param  ndef_msg_len The length of the encoded NDEF message.
 *
 * @return 0 if successful, otherwise an error code.
 */
int nfc_Encode_Launch_App_Msg(const uint8_t *url, size_t url_len, uint8_t *ndef_msg_buf, size_t *ndef_msg_len)
{
    int err = nfc_launchapp_msg_encode(NULL, 0, url, url_len, ndef_msg_buf, ndef_msg_len);
    return err;
}
 
/**
 * @brief  Sets the NFC payload.
 *
 *         This function sets the NFC payload with the provided NDEF message buffer
 *         and its length.
 *
 * @param  ndef_msg_buf  The NDEF message buffer.
 * @param  ndef_msg_len  The length of the NDEF message.
 *
 * @return 0 if successful, otherwise an error code.
 */
int nfc_Set_Payload(uint8_t *ndef_msg_buf, size_t ndef_msg_len)
{
    int err = nfc_t2t_payload_set(ndef_msg_buf, ndef_msg_len);
    return err;
}
 
/**
 * @brief  Starts NFC Type 2 Tag emulation.
 *
 *         This function starts the NFC Type 2 Tag emulation.
 *
 * @return 0 if successful, otherwise an error code.
 */
int nfc_Start_Emulation(void)
{
    int err = nfc_t2t_emulation_start();
    return err;
}