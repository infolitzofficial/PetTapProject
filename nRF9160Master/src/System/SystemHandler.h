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
    WIFI_CONNECTED,
    WAIT_CONNECTION,
    BLE_CONNECTED,
    WIFI_DEVICE,
    BLE_DEVICE,
    DEVICE_IDLE,
}_eDevState;

typedef struct __sGnssConfig
{
    double dLatitude;
    double dLongitude;
    bool bLocationUpdated;
}_sGnssConfig;

/**********************************************FUNCTION DECLARATIONS*************************************/
void ProcessDeviceState();
void PollMsgs();
_eDevState *GetDeviceState();
void SetDeviceState(_eDevState DeviceState);
bool IsLocationDataOK(void);
void SetLocationDataStatus(bool bStatus);
bool UpdateLocation(_sGnssConfig *psLocationData);
void InitTimerTask(int nPeriod);
_sGnssConfig * GetLocationData();

#endif