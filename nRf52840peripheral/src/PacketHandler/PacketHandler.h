/**
 * @file    : LoRaE32Handler.h
 * @brief   : Functions for handling LoRa E32
 * @author  : Jeslin James
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
#define START_BYTE      "*"
#define END_BYTE        "#"

/*********************************************************TYPEDEFS************************************************/

typedef enum __ePacketType
{
    CMD,
    RESP,
    DATA,
    ACK
}_ePacketType;

typedef struct __sPacket
{
    uint8_t ucStartByte;
    _ePacketType PacketType;
    uint8_t *pucPayload;
    uint16_t usLen;
    uint8_t ucEndByte;
}_sPacket;

/*********************************************************FUNCTION DECLARATION************************************/
bool BuildPacket(_sPacket *psPacket,_ePacketType PcktType, uint8_t *pucPayload, uint16_t usLen);
bool ParsePacket(uint8_t *pucRcvdBuffer, _sPacket *psPacket);
bool ProcessRcvdPacket(_sPacket *psPacket);
bool ProcessCmd(char *pcCmd);
bool ProcessResp(char *pcResp);
bool ProcessPayload(char *pcPayload);
bool ProcessAcknowledge(char *pcMsg);

#endif

//EOF