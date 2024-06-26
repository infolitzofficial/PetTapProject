/**
 * @file    : SystemHandler.h
 * @brief   : File contains System handler related functions
 * @author  : Adhil
 * @date    : 06-03-2024
 * @see     : SystemHandler.c
 * @note
*/

#ifndef _SYSTEM_HANDLER_H
#define _SYSTEM_HANDLER_H
/*********************************************INCLUDES***************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*********************************************TYPEDEFS***************************************************/
typedef enum __eDevState
{
    BLE_IDLE,
    BLE_CONN_REQ,
    BLE_CONNECTED,
    BLE_DISCONNECTED,
    BLE_CONFIG
}_eDevState;

/**********************************************FUNCTION DECLARATIONS*************************************/
void ProcessDeviceState();
void PollMsgs();
_eDevState *GetDeviceState();
void SetDeviceState(_eDevState DeviceState);

#endif

//EOF