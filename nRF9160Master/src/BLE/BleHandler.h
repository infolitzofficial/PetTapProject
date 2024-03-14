/**
 * @file    : WiFiHandler.h
 * @brief   : File contains WiFi related functions
 * @author  : Adhil
 * @date    : 06-03-2024
 * @see     : WiFiHandler.c
 * @note
*/

#ifndef _BLE_HANDLER_H
#define _BLE_HANDLER_H

/*********************************************INCLUDES***************************************************/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*********************************************************MACROS**************************************************/
#define BUFFER_SIZE           1024

/*********************************************************TYPEDEFS************************************************/

typedef enum __eUartRxState
{
    START,
    RCV,
    END
}_eUartRxState;

/**********************************************TYPEDEFS***************************************************/


/***********************************************FUNCTION DECLARATIONS**************************************/
bool InitBleUart(void);
void SendBleMsg(uint8_t *pucBuff, uint16_t usLen);
bool ReadPacket(uint8_t *pucBuffer);
bool ReadBuffer(void);
bool SendLocationToBle();
#endif

//EOF