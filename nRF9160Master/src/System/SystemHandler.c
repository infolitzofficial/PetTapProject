/**
 * @file   : SystemHandler.c
 * @brief  : Files handling system behaviour
 * @author : Adhil
 * @date   : 05-03-2024
 * @ref    : SystemHandler.h
*/

/******************************************INCLUDES*****************************/
#include "SystemHandler.h"
#include "../WiFi/WiFiHandler.h"

/******************************************MACROS******************************/


/******************************************TYPEDEFS****************************/
static _eDevState DevState = DEVICE_IDLE;

/*****************************************FUNCTION DEFINITION******************/
void ProcessDeviceState()
{
    uint32_t TimeNow = 0;

    switch(DevState)
    {
        case DEVICE_IDLE:
                        //Perform Connection

                       // TimeNow = sys_clock_tick_get();
                       printk("INFO: IDLE STATE\n\r");


                        if (ConfigureAndConnectWiFi())
                        {
                            DevState = WAIT_CONNECTION;
                        }
                        else
                        {
                            printk("INFO: WiFi Conn failed\n\r");
                        }

                        break;
        case WAIT_CONNECTION:
                        printk("INFO: CONN WAIT\n\r");
                        if(IsWiFiConnected())
                        {
                            DevState = WIFI_CONNECTED;
                        }
        case WIFI_CONNECTED:
                        printk("INFO: WIFI CONNECTED\n\r");
                        DevState = WIFI_DEVICE;
                        break;
        case WIFI_DEVICE:
                       
                        printk("INFO: WIFI MODE\n\r");
                       //Do wifi operation
                       //if Disconnected switch to DEVICE_IDLE
                       break;
        case BLE_CONNECTED:
                        DevState = BLE_DEVICE;
                        break;
        case BLE_DEVICE:
                       //Do BLE operation
                       //if Disconnected switch to DEVICE_IDLE
                       break;
    }
}

/**
 * @brief
*/
_eDevState *GetDeviceState()
{
    return &DevState;
}

