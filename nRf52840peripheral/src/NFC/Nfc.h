/**
 * @file    NFC.h
 * @author : Devendu
 * @brief   File containing NFC functionalities
 * @date    22-03-2023
 * @see     NFC.c
 */
 
#ifndef NFC_H
#define NFC_H
 
/**************************************INCLUDES******************************/
#include <stddef.h>
#include <stdint.h>
 
/***************************************MACROS*******************************/
#define ADV_BUFF_SIZE           (100)
 
/**************************************FUNCTION DECLARATIONS****************/
int nfc_Setup(void);
int nfc_Encode_Launch_App_Msg(const uint8_t *url, size_t url_len, uint8_t *ndef_msg_buf, size_t *ndef_msg_len);
int nfc_Set_Payload(uint8_t *ndef_msg_buf, size_t ndef_msg_len);
int nfc_Start_Emulation(void);
 
#endif /* NFC_H */