/**
 * @file    WiFiHandler.h
 * @brief   WiFi related functions are defined here
 * @author  Jeslin
 * @date    15-01-2024
 * @see     WiFIHandler.c
*/

#ifndef _SYSTEM_HANDLER_H
#define _SYSTEM_HANDLER_H
/***************************INCLUDES*********************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*****************************TYPEDEFS******************************/
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

/***********************************FUNCTION DECLARATIONS******************/
void ProcessDeviceState();
_eDevState *GetDeviceState();
bool IsLocationDataOK(void);
void SetLocationDataStatus(bool bStatus);
bool UpdateLocation(_sGnssConfig *psLocationData);
void InitTimerTask(int nPeriod);
_sGnssConfig * GetLocationData();

#endif