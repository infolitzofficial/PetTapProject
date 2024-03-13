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

/*********************************************MACROS******************************************************/
// #define TICK_RATE               32768
// #define TIMESLOT       TICK_RATE * 15

/**********************************************TYPEDEFS***************************************************/


/***********************************************FUNCTION DECLARATIONS**************************************/
bool InitBleUart(void);
void SendBleMsg(uint8_t *pucBuff, uint16_t usLen);

#endif

//EOF