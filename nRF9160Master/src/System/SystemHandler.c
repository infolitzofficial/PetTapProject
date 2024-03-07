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
_sGnssConfig sGnssConfig = {0.0,0.0,false};
struct k_timer Timer;
static bool TimerExpired = false;

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
                        InitTimerTask(30);
                        DevState = WIFI_DEVICE;
                        break;
        case WIFI_DEVICE:
                        if (TimerExpired)
                        {
                            if (IsLocationDataOK())
                            {
                                if (SendLocation())
                                {
                                    printk("INFO: Location sent success\n\r");
                                    TimerExpired=false;
                                }
                            }
                        }
                       // printk("INFO: WIFI MODE\n\r");
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

/**
 * @brief 
*/
_sGnssConfig * GetLocationData()
{
    return &sGnssConfig;
}

/**
 * @brief 
*/
bool UpdateLocation(_sGnssConfig *psLocationData)
{
    bool bRetVal = false;

    if (psLocationData)
    {
        sGnssConfig.dLatitude  = psLocationData->dLatitude;
        sGnssConfig.dLongitude = psLocationData->dLongitude;
        bRetVal = true;
    }

    return bRetVal;
}

/**
 * @brief
*/
bool IsLocationDataOK(void)
{
    return sGnssConfig.bLocationUpdated;
}

/**
 * @brief
*/
void SetLocationDataStatus(bool bStatus)
{
    sGnssConfig.bLocationUpdated = bStatus;
}

/**
 * @brief
*/
static void TimerExpiredCb(struct k_timer *timer)
{
    //Callback for sending data over Wifi/BLE/LTE
    switch(DevState)
    {
        case WIFI_DEVICE :  printk("INFO: TimerExpired Callback\n\r");
                            //k_msleep(50);
                            TimerExpired = true;
                            break;

        case BLE_DEVICE  :  
                            // if (IsLocationDataOK())
                            // {
                            //     //Send Data to nRf52840                               
                            // }
                            break;

        default          :
                            break;
    }
}

/**
 * @brief
*/
static void TimerStoppedCb(struct k_timer *timer)
{
    printk("INFO timer stopped\n\r");
}

/**
 * @brief
*/
void InitTimerTask(int nPeriod)
{
    k_timer_init(&Timer, TimerExpiredCb, TimerStoppedCb);
    k_timer_start(&Timer, K_SECONDS(0), K_SECONDS(nPeriod));
}

/**
 * @brief
*/
void StopTimer()
{
    k_timer_stop(&Timer);
}