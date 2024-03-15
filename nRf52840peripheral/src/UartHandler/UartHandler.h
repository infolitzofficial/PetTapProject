/**
 * @file    : UartHandler.h
 * @brief   : Functions for handling uart channel in 52840
 * @author  : Adhil
 * @date    : 13-03-2024
*/

#ifndef _UART_HANDLER_H
#define _UART_HANDLER_H

/*********************************************************INCLUDES************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/gpio.h>

/*********************************************************MACROS**************************************************/
#define BUFFER_SIZE           1024

/*********************************************************TYPEDEFS************************************************/

typedef enum __eUartRxState
{
    START,
    RCV,
    END
}_eUartRxState;

/*********************************************************FUNCTION DECLARATION************************************/
bool InitUart(void);
void ReceptionCb(const struct device *dev, void *user_data);
bool ReadBuffer(void);
void SendData(const uint8_t *pcData, uint16_t usLength);
bool ReadPacket(uint8_t *pucBuffer);

#endif

//EOF