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
#include "../NVS/NvsHandler.h"
#include "zephyr/sys/printk.h"
#include <sys/_stdint.h>

/*******************************************MACROS**********************************************************/
#define BUFFER_SIZE     255

/******************************************TYPEDEFS*********************************************************/
static _eDevState DevState = DEV_IDLE;
_sGnssConfig sGnssConfig = {0.0,0.0,false};
static bool bConfigStatus = false;
struct k_timer Timer;
static bool TimerExpired = false;
static long long llSysTick = 0;
static uint8_t uCredIdx = 0;

/*****************************************FUNCTION DEFINITION***********************************************/
/**
 * @brief       : Connect to a 52840 device 
 * @param [in]  : None
 * @param [out] : none
 * @return      : true for success
*/
static bool ConnectToBLE()
{
    uint8_t ucPayload[BUFFER_SIZE] = {0};
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
void ProcessBleMsg()
{
    uint32_t TimeNow = 0;
    uint8_t ucBuff[BUFFER_SIZE];
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
    int nRetry = 2;
    long long llCurrentTick = 0;
    _sAtCmdHandle *psAtCmdHndler = NULL;
#ifdef NVS_ENABLE
    _sConfigData *psConfigData = NULL;
    char uReadBuf[256] = {0};

    psConfigData = GetConfigData();
#endif
    psAtCmdHndler = GetATCmdHandle();

    switch(DevState)
    {
        case DEV_IDLE:
                    //Perform Configuration
                    printk("INFO: IDLE STATE\n\r");
                    llSysTick = sys_clock_tick_get();

                    do
                    {
                        if (ConfigureWiFi())
                        {
                            bConfigStatus = true;
                            break;
                        }
                        else
                        {
                            printk("ERR: WiFi Conn failed\n\r");
                        }
                    } while (nRetry-- > 0);

                    SetDeviceState(WAIT_CONNECTION);
                    break;

        case WAIT_CONNECTION:
                    printk("Info: Sending connection request to BLE\n\r");
                    llCurrentTick = sys_clock_tick_get();
                    if (ConnectToBLE())
                    {
                        SetDeviceState(WAIT_CONNECTION);
                    }
                    else
                    {
                        printk("ERR: BLE Conn failed\n\r");                        
                    }

                    if (!bConfigStatus)
                    {
                        SetDeviceState(DEV_IDLE);
                    }

                    if ((llCurrentTick - llSysTick) >= (20 * 32768)) 
                    {
                        sprintf(uReadBuf, "%s,%s", psConfigData[uCredIdx].sWifiCred.ucSsid, psConfigData[uCredIdx].sWifiCred.ucPwd);
                        SetAPCredentials(uReadBuf);
                        printk("Send APCredentials request to DA***************************%s\n\r", uReadBuf);
                        psAtCmdHndler[2].CmdHdlr(psAtCmdHndler[2].pcCmd, psAtCmdHndler[2].pcArgs, psAtCmdHndler[2].nArgsCount);
                        if (uCredIdx > 5) 
                        {
                            uCredIdx = 1;
                        }
                        else 
                        {
                            uCredIdx++;
                        }
                        llSysTick = llCurrentTick;
                    }

                    printk("INFO: waiting connection\n\r");
                    k_msleep(500);
                    break;

        case WIFI_CONNECTED:
                    printk("INFO: Connected to WiFi\n\r");
                    StarTimerTask(30);
                    SetDeviceState(WIFI_DEVICE);
                    break;

        case WIFI_DEVICE:
                    if (TimerExpired)
                    {   if (IsWiFiConnected())
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
                    }

                    k_msleep(500);
                    break;

        case WIFI_DISCONNECTED:
                    printk("INFO: AP is not visble\n\r");
                    DisconnectFromWiFi();
                    StopTimer();
                    SetDeviceState(WAIT_CONNECTION);
                    break;

        case BLE_CONNECTED:
                    printk("INFO: Connected to BLE\n\r");
                    SetDeviceState(BLE_DEVICE);
                    break;

        case BLE_DEVICE:
                    //No Operation
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
 * @param [in]  : DeviceState
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
                            TimerExpired = true;
                            break;

        case BLE_DEVICE  :  
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
void InitTimerTask()
{
    k_timer_init(&Timer, TimerExpiredCb, TimerStoppedCb);
    
}

void StarTimerTask(int nPeriod)
{
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

//EOF