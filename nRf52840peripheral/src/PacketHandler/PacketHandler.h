/**
 * @file    : PacketHandler.h
 * @brief   : Functions for handling Packet mechanism
 * @author  : Adhil
 * @date    : 02-03-2024
*/

#ifndef _PACKET_HANDLER_H
#define _PACKET_HANDLER_H

/*********************************************************INCLUDES************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*********************************************************MACROS**************************************************/
#define START_BYTE      0x2A
#define END_BYTE        0x23

/*********************************************************TYPEDEFS************************************************/

typedef enum __ePacketType
{
    CMD  = 0,
    RESP,
    DATA,
    ACK
}_ePacketType;

//Note: Do not modify below structure it might affect communication between 9160
typedef struct __attribute__((__packed__)) __sPacket
{
    uint8_t ucStartByte;
    _ePacketType PacketType;
    uint8_t pucPayload[100];
    uint16_t usLen;
    uint8_t ucEndByte;
}_sPacket;

/*********************************************************FUNCTION DECLARATION************************************/
bool BuildPacket(_sPacket *psPacket,_ePacketType PcktType, 
                uint8_t *pucPayload, uint16_t usPayloadLen);
bool ParsePacket(uint8_t *pucRcvdBuffer, _sPacket *psPacket);
bool ProcessRcvdPacket(_sPacket *psPacket);
bool ProcessCmd(char *pcCmd);
bool ProcessResponse(char *pcResp);
bool ProcessPayload(char *pcPayload);

#endif

//EOF