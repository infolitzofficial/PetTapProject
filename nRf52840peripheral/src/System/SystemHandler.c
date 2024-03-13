/**
 * @file   : SystemHandler.c
 * @brief  : Files handling system behaviour
 * @author : Adhil
 * @date   : 05-03-2024
 * @ref    : SystemHandler.h
*/

/*******************************************INCLUDES********************************************************/
#include "SystemHandler.h"
#include "../PacketHandler/PacketHandler.h"
#include "../UartHandler/UartHandler.h"
#include "../BLE/BleHandler.h"
#include "../BLE/BleService.h"

/*******************************************MACROS**********************************************************/


/******************************************TYPEDEFS*********************************************************/
static _eDevState DevState = DEVICE_IDLE;

/*****************************************FUNCTION DEFINITION***********************************************/
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


void ProcessDeviceState()
{
    switch(DevState)
    {
        case DEVICE_IDLE:
                    //Perform Connection
                    break;

        case DEVICE_CONNECTED:
                    if (IsNotificationenabled())
                    {
                        
                    }
                    break;

        case DEVICE_DISCONNECTED:
                    //Do wifi operation
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
 * @brief       : Set device state
 * @param [in]  : DeviceState - device state to set
 * @param [out] : none
 * @return      : None
*/
void SetDeviceState(_eDevState DeviceState)
{
    DevState = DeviceState;
}