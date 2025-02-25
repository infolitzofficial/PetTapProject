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
static _eDevState DevState = BLE_IDLE;
static const struct gpio_dt_spec sEnablePin = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(gpsenable), gpios, {0});
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
        printk("Received packet %s\n\r", ucBuff);

        if (ParsePacket(ucBuff, &sPacket))
        {
            ProcessRcvdPacket(&sPacket);
        }
    }
}


void ProcessDeviceState()
{
    _sPacket sPacket = {0};
    uint8_t ucPayload[255];

    switch(DevState)
    {
        case BLE_IDLE:
                    //No Operation 
                    break;

        case BLE_CONN_REQ:
                   // Ble Connection Request
                    if (IsConnected())
                    {
                        strcpy((char *)ucPayload, "ACK");
                        SetDeviceState(BLE_CONNECTED);
                        BuildPacket(&sPacket, RESP, ucPayload, strlen((char *)ucPayload));
                    }
                    else
                    {
                        strcpy((char *)ucPayload, "NACK");
                        SetDeviceState(BLE_IDLE);
                        BuildPacket(&sPacket, RESP, ucPayload, strlen((char *)ucPayload));
                    }
                    SendData((uint8_t *)&sPacket, sizeof(sPacket));
                    break;
        case BLE_CONNECTED:
                    if (IsNotificationenabled())
                    {
                        strcpy((char *)ucPayload, "LOCATION");
                        BuildPacket(&sPacket, CMD, ucPayload, strlen((char *)ucPayload));
                        SendData((uint8_t *)&sPacket, sizeof(sPacket));
                    }
                    break;

        case BLE_DISCONNECTED:
                    strcpy((char *)ucPayload, "DISCONNECT");
                    SetDeviceState(BLE_IDLE);
                    BuildPacket(&sPacket, CMD, ucPayload, strlen((char *)ucPayload));
                    SendData((uint8_t *)&sPacket, sizeof(sPacket));
                    break;
        case BLE_CONFIG:
                    GetRcvdData(ucPayload);
                    printk("chk- test payload- %s\n", ucPayload);
                    BuildPacket(&sPacket, CMD, ucPayload, strlen((char *)ucPayload));
                    SendData((uint8_t *)&sPacket, sizeof(sPacket));
                    SetDeviceState(BLE_CONNECTED);                  //chk
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

bool InitEnable9160()
{
    bool bRetVal = false;

    do
    {
        if (!device_is_ready(&sEnablePin)) 
        {
            printf("E: PowerPin reference not available\n\r");
            break;
        }

        if (gpio_pin_configure_dt(&sEnablePin, GPIO_OUTPUT | GPIO_ACTIVE_HIGH) < 0)
        {
            printf("E: PowerPin configuring failed\n\r");
            break;
        }


        // gpio_pin_set_dt(psPowerPin, 1);
        gpio_pin_set(sEnablePin.port, sEnablePin.pin, 1);
        // if (gpio_pin_set_dt(psPowerPin, ))
        bRetVal = true;

    } while (0);
        

    return bRetVal;
    // DEVICE_DT_GET(DT_NODE)
}
//EOF