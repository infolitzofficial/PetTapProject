/**
 * @file    GPSHandler.h
 * @brief   GPS related functions are handled here
 * @author  Adhil
 * @date    14-10-2023
 * @see     GSPHandler.c
*/
#ifndef _GPS_HANDLER_H
#define _GPS_HANDLER_H

/***************************INCLUDES*********************************/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/***************************MACROS**********************************/
#define GPS_LOC_OK        ~(1<<4)
#define GPS_LOC_FAILED     (1<<4)

/***************************TYPEDEFS********************************/

/***************************FUNCTION DECLARATION*********************/
bool InitUart(void);
void SendData(const char *pcData);

#endif

//EOF