/**
 * @file    Nfc.h
 * @author : divin raj
 * @brief   File containing NFC functionalities
 * @date    02-04-2024
 * @see     Nfc.c
 */
 
#ifndef NFC_H
#define NFC_H
 
/**************************************INCLUDES******************************/
#include <stddef.h>
#include <stdint.h>
 
/***************************************MACROS*******************************/
#define ADV_BUFF_SIZE           (100)
 
/**************************************FUNCTION DECLARATIONS****************/
int NFCSetup(void);
int NFCEncodeLaunchAppMsg(const uint8_t *uURL, size_t ulUrlLen, uint8_t *puNFCMsgBuf, size_t *pulNFCMsgLen);
int NFCSetPayLoad(uint8_t *ndef_msg_buf, size_t ndef_msg_len);
int NFCStartEmulation(void);
 
#endif /* NFC_H */