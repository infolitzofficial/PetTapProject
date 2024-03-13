/**
 * @file   : SystemHandler.c
 * @brief  : Files handling system behaviour
 * @author : Adhil
 * @date   : 05-03-2024
 * @ref    : SystemHandler.h
*/

/*******************************************INCLUDES********************************************************/
#include "SystemHandler.h"
#include "../WiFi/WiFiHandler.h"
#include "../PacketHandler/PacketHandler.h"

/*******************************************MACROS**********************************************************/


/******************************************TYPEDEFS*********************************************************/
static _eDevState DevState = DEVICE_IDLE;
_sGnssConfig sGnssConfig = {0.0,0.0,false};
struct k_timer Timer;
static bool TimerExpired = false;

/*****************************************FUNCTION DEFINITION***********************************************/
/**
 * @brief       : Connect to a 52840 device 
 * @param [in]  : None
 * @param [out] : none
 * @return      : true for success
*/
static bool ConnectToBLE()
{
    uint8_t ucPayload[255] = {0};
    _sPacket sPacket = {0};
    bool bRetVal = false;

    strcpy((char *)ucPayload, "CONNECT");

    if (BuildPacket(&sPacket, CMD, ucPayload, strlen((char *)ucPayload)))
    {
        SendBleMsg((uint8_t *)&sPacket, sizeof(_sPacket));
        bRetVal = true;
    }

    return bRetVal;
}

/**
 * @brief       : Process Device state of MASTER device
 * @param [in]  : None
 * @param [out] : none
 * @return      : None
*/
void PollMsgs()
{
    uint32_t TimeNow = 0;
    uint8_t ucBuff[255];
    _sPacket sPacket = {0};

    if (ReadPacket(ucBuff))
    {
        printk("Received packet\n\r");

        if (ParsePacket(ucBuff, &sPacket))
        {
            ProcessRcvdPacket(&sPacket);
        }
    }
}

/**
 * @brief       : Process Device state of MASTER device
 * @param [in]  : None
 * @param [out] : none
 * @return      : None
*/
void ProcessDeviceState()
{
    uint32_t TimeNow = 0;

    PollMsgs();

    switch(DevState)
    {
        case DEVICE_IDLE:
                    //Perform Connection
                    printk("INFO: IDLE STATE\n\r");

                    // if (ConfigureAndConnectWiFi())
                    // {
                    //     DevState = WAIT_CONNECTION;
                    // }
                    // else
                    // {
                    //     printk("ERR: WiFi Conn failed\n\r");
                    // }

                    printk("Info: befor ble connect\n\r");
                    if (ConnectToBLE())
                    {
                        DevState = WAIT_CONNECTION;
                    }
                    else
                    {
                        printk("ERR: BLE Conn failed\n\r");                        
                    }

                    break;

        case WAIT_CONNECTION:
                    //printk("INFO: CONN WAIT\n\r");
                    if(IsWiFiConnected())
                    {
                        DevState = WIFI_CONNECTED;
                    }
                    break;

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

        default        :
                    break;
    }
}

/**
 * @brief       : Get current device state
 * @param [in]  : None
 * @param [out] : none
 * @return      : Current device state
*/
_eDevState *GetDeviceState()
{
    return &DevState;
}

/**
 * @brief       : Set current device state
 * @param [in]  : DeviceState - DeviceState
 * @param [out] : none
 * @return      : Current device state
*/
void SetDeviceState(_eDevState DeviceState)
{
    DevState = DeviceState;
}

/**
 * @brief       : Get location data 
 * @param [in]  : None
 * @param [out] : none
 * @return      : Current GNSS config containing location data
 *                (Will update the name of return parameter later)
*/
_sGnssConfig * GetLocationData()
{
    return &sGnssConfig;
}

/**
 * @brief       : Update location read from GNSS to GNSS config
 * @param [in]  : None
 * @param [out] : psLocationData - Location data
 * @return      : true for success
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
 * @brief       : Checking received location data is valid
 * @param [in]  : None
 * @param [out] : None
 * @return      : true for success
*/
bool IsLocationDataOK(void)
{
    return sGnssConfig.bLocationUpdated;
}

/**
 * @brief       : Setting status of GNSS location fix
 * @param [in]  : bStatus - status of the location fix
 * @param [out] : None
 * @return      : true for success
*/
void SetLocationDataStatus(bool bStatus)
{
    sGnssConfig.bLocationUpdated = bStatus;
}

/**
 * @brief       : Timer task expired callback
 * @param [in]  : None
 * @param [out] : timer - timer handle
 * @return      : None
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
 * @brief       : Timer task stopped callback
 * @param [in]  : None
 * @param [out] : timer - timer handle
 * @return      : None
*/
static void TimerStoppedCb(struct k_timer *timer)
{
    printk("INFO timer stopped\n\r");
}

/**
 * @brief       : Initialise and start Timer task 
 * @param [in]  : nPeriod - period of timer 
 * @param [out] : None
 * @return      : None
*/
void InitTimerTask(int nPeriod)
{
    k_timer_init(&Timer, TimerExpiredCb, TimerStoppedCb);
    k_timer_start(&Timer, K_SECONDS(0), K_SECONDS(nPeriod));
}

/**
 * @brief       : Stop Timer task 
 * @param [in]  : None 
 * @param [out] : None
 * @return      : None
*/
void StopTimer()
{
    k_timer_stop(&Timer);
}