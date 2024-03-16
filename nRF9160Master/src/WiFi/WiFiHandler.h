/**
 * @file    : WiFiHandler.h
 * @brief   : File contains WiFi related functions
 * @author  : Adhil
 * @date    : 06-03-2024
 * @see     : WiFiHandler.c
 * @note
*/

#ifndef _WIFI_HANDLER_H
#define _WIFI_HANDLER_H

/*********************************************INCLUDES***************************************************/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*********************************************MACROS******************************************************/
#define TICK_RATE      32768
#define TIMESLOT       TICK_RATE * 15
#define ARGS_CNT       5

/**********************************************TYPEDEFS***************************************************/
typedef void (*cmdHandler)(const char *pcCmd, char *pcArgs[], int nArgc);
typedef void (*respHandler)(const char *pcResp, bool *pbStatus);

typedef enum __eUartRxState
{
    START,
    RCV,
    END
}_eUartRxState;
typedef struct __sAtCmdHandle
{
    const char *pcCmd;
    cmdHandler CmdHdlr;
    respHandler RespHdlr;
    int  nArgsCount;
    char *pcArgs[ARGS_CNT];
}_sAtCmdHandle;

/***********************************************FUNCTION DECLARATIONS**************************************/
bool InitUart(void);
void ProcessResponse(const char *pcResp, bool *pbStatus);
bool ConfigureWiFi();
bool IsWiFiConnected();
void ProcessWiFiMsgs();
bool SendLocation();
bool ReadBuff(void);
bool DisconnectFromWiFi();
#endif

//EOF