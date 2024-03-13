/**
 * @file    : PacketHandler.c
 * @brief   : Functions for handling uart peripheral
 * @author  : Adhil
 * @date    : 13-03-2024
*/
/*******************************************************INCLUDES***************************************************/
#include "PacketHandler.h"
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
bool BuildPacket(_sPacket *psPacket,_ePacketType PcktType, uint8_t *pucPayload, uint16_t usLen)
{
    bool bRetVal = false;

    if (psPacket && pucPayload)
    {
        psPacket->ucStartByte = START_BYTE;
        psPacket->PacketType = PcktType;
        psPacket->pucPayload = (uint8_t *)malloc(usLen);
        memcpy(psPacket->pucPayload, pucPayload, usLen);
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
            case CMD : //ProcessCmd
                    break;
            case RESP: //ProcessResponse
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
        if (strcmp(pcCmd, "CONNECT") == 0)
        {
#ifdef nRF52840            
            SetDeviceState(DEVICE_CONNECTED);
#endif
        }
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
        if (strcmp(pcResp, "CONN_OK") == 0)
        {
            //Process Response
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

/**
 * @brief      : Process acknowledge
 * @param [in] : pcMsg - acknowledgement message
 * @param [out]: None
 * @return     : true for success
*/
bool ProcessAcknowledge(char *pcMsg)
{
    bool bRetVal = false;
    _eDevState *pDevState = NULL;

    if (pcMsg)
    {
        if (strcmp(pcMsg, "OK") == 0)
        {
            bRetVal = true;
        }
    }

    return bRetVal;
}