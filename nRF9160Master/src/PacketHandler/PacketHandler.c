/**
 * @file    : PacketHandler.c
 * @brief   : Functions for handling Packet mechanism
 * @author  : Adhil
 * @date    : 13-03-2024
*/
/*******************************************************INCLUDES***************************************************/
#include "PacketHandler.h"
#include "../BLE/BleHandler.h"
#include "../System/SystemHandler.h"

/*******************************************************MACROS*****************************************************/

/*******************************************************TYPEDEFS***************************************************/

/*******************************************************PRIVATE VARIABLES******************************************/

/*******************************************************PUBLIC VARIABLES*******************************************/

/*******************************************************FUNCTION DEFINITION*****************************************/


/**
 * @brief      : Build Packet to send
 * @param [in] : usLen - Length of the packet
 *             : PcktType - PacketType
 * @param [out]: psPacket - Packet to build
 *             : pucPayload - Payload to send
 * @return     : returns true on success
*/
bool BuildPacket(_sPacket *psPacket,_ePacketType PcktType, 
                uint8_t *pucPayload, uint16_t usPayloadLen)
{
    bool bRetVal = false;
    
    printk("Length of payload: %d\n\r", usPayloadLen);
    if (psPacket && pucPayload)
    {
        psPacket->ucStartByte = START_BYTE;
        psPacket->PacketType = PcktType;
        memcpy(psPacket->pucPayload, pucPayload, usPayloadLen);
        psPacket->usLen = usPayloadLen;
        psPacket->ucEndByte = END_BYTE;
        bRetVal = true;
    }

    return bRetVal;
}

/**
 * @brief      : Parse packet received
 * @param [in] : None
 * @param [out]: pucRcvdBuffer - Receieved Buffer
 *             : psPacket - Packet received
 * @return     : true for success
*/
bool ParsePacket(uint8_t *pucRcvdBuffer, _sPacket *psPacket)
{
    bool bRetVal = false;

    if (pucRcvdBuffer && psPacket)
    {
        memcpy(psPacket, pucRcvdBuffer, sizeof(_sPacket));
        bRetVal = true;
    }

    return bRetVal;
}

/**
 * @brief      : Parse packet received
 * @param [in] : None
 * @param [out]: pucRcvdBuffer - Receieved Buffer
 *             : psPacket - Packet received
 * @return     : true for success
*/
bool ProcessRcvdPacket(_sPacket *psPacket)
{
    bool bRetVal = false;

    if (psPacket)
    {
        switch (psPacket->PacketType)
        {
            case CMD : ProcessCmd(psPacket->pucPayload);
                    break;
            case RESP: ProcessResp(psPacket->pucPayload);
                    break;
            case DATA: //ProcessData
                    break;
            case ACK:  //ProcessAcknowlodge
                    break;
            default  :
                    break;
        }

        bRetVal = true;
    }

    return bRetVal;
}

/**
 * @brief      : Process command
 * @param [in] : 
 * @param [out]: None
 * @return     : true for success
*/
bool ProcessCmd(char *pcCmd)
{
    bool bRetVal = false;

    if (pcCmd)
    {
#ifdef nRF52840    
        if (strcmp(pcCmd, "CONNECT") == 0)
        {        
            SetDeviceState(DEVICE_CONNECTED);

        }
#endif
        if (strcmp(pcCmd, "DISCONNECT") == 0)
        {
            SetDeviceState(DEV_IDLE);
        }
        else if(strcmp(pcCmd, "LOCATION") == 0)
        {
            if (IsLocationDataOK())
            {
                SendLocationToBle();
            }
            else
            {
                printk("Didnt get location fix\n\r");
            }
        }
    }
}

static void UpdateStateAfterResponse(bool bStatus)
{
    _eDevState *pDevState = 0;

    pDevState = GetDeviceState();

    switch(*pDevState)
    {
        case WAIT_CONNECTION:
                            if (bStatus)
                            {
                                SetDeviceState(BLE_CONNECTED);
                            }
                            else
                            {
                                SetDeviceState(DEV_IDLE);
                            }
                            break;

        default             :
                            break;
    }
}

/**
 * @brief      : Process response
 * @param [in] : None
 * @param [out]: None
 * @return     : true for success
*/
bool ProcessResp(char *pcResp)
{
    bool bRetVal = false;

    if (pcResp)
    {
        if (strcmp(pcResp, "ACK") == 0)
        {
            UpdateStateAfterResponse(true);
        }
        else if (strcmp(pcResp, "NACK") == 0)
        {
            UpdateStateAfterResponse(false);
        }

        bRetVal = true;
    }

    return bRetVal;
}

/**
 * @brief      : Process payload
 * @param [in] : pcPayload - payload
 * @param [out]: None
 * @return     : true for success
*/
bool ProcessPayload(char *pcPayload)
{
    bool bRetVal = false;

    if (pcPayload)
    {
        bRetVal = true;
    }

    return bRetVal;
}

//EOF