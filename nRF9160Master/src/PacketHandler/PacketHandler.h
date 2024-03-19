/**
 * @file    : PacketHandler.h
 * @brief   : Functions for handling Packet mechanism
 * @author  : Adhil
 * @date    : 13-03-2024
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
#define DATA_SIZE       100

/*********************************************************TYPEDEFS************************************************/

typedef enum __ePacketType
{
    CMD  = 0,
    RESP,
    DATA,
    ACK
}_ePacketType;

typedef struct __attribute__((__packed__)) __sPacket
{
    uint8_t ucStartByte;
    _ePacketType PacketType;
    uint8_t pucPayload[DATA_SIZE];
    uint16_t usLen;
    uint8_t ucEndByte;
}_sPacket;

/*********************************************************FUNCTION DECLARATION************************************/
bool BuildPacket(_sPacket *psPacket,_ePacketType PcktType, 
                uint8_t *pucPayload, uint16_t usPayloadLen);
bool ParsePacket(uint8_t *pucRcvdBuffer, _sPacket *psPacket);
bool ProcessRcvdPacket(_sPacket *psPacket);
bool ProcessCmd(char *pcCmd);
bool ProcessResp(char *pcResp);
bool ProcessPayload(char *pcPayload);
void parseWifiCred(const char *pcCmd);
const char* getNewCred();
#endif

//EOF