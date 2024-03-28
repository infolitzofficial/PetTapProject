/**
 * @file    : PacketHandler.c
 * @brief   : Functions for handling uart peripheral
 * @author  : Adhil
 * @date    : 13-03-2024
*/
/*******************************************************INCLUDES***************************************************/
#include "PacketHandler.h"
#include <ctype.h>
#include "../System/SystemHandler.h"

/*******************************************************MACROS*****************************************************/
#define nRF52840
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
        printk("Inside parse packet\n\r");
        for (int i=0; i <=sizeof(_sPacket); i++)
        {
            printk("%02x ", pucRcvdBuffer[i]);
        }
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
            case RESP: ProcessResponse(psPacket->pucPayload);
                    break;
            case DATA: //ProcessData
                    break;
            case ACK:  //ProcessAcknowledge(psPacket->pucPayload);
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
    printk("\n 528ProcessCmd\n");
    bool bRetVal = false;

    if (pcCmd)
    {
        if (strcmp(pcCmd, "CONNECT") == 0)
        {
#ifdef nRF52840            
            SetDeviceState(BLE_CONN_REQ);
#endif
        }
        else {
            printk("/n In here: %s\n", pcCmd);
        }
    }
}

/**
 * @brief      : Process response
 * @param [in] : None
 * @param [out]: None
 * @return     : true for success
*/
bool ProcessResponse(char *pcResp)
{
    bool bRetVal = false;
    printk("\n 528ProcessResp\n");
    if (pcResp)
    {
        if (strcmp(pcResp, "ACK") == 0)
        {
            bRetVal = true;
        }
        else
        {
            LocationdataNotify(pcResp, strlen(pcResp));
        }
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