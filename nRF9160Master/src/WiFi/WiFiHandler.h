/**
 * @file    GSMHandler.h
 * @brief   GSM related functions are defined here
 * @author  Jeslin
 * @date    15-01-2024
 * @see     GSMHandler.c
*/

#ifndef _WIFI_HANDLER_H
#define _WIFI_HANDLER_H
/***************************INCLUDES*********************************/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define TICK_RATE               32768
#define TIMESLOT       TICK_RATE * 15

typedef void (*cmdHandler)(const char *pcCmd, char *pcArgs[], int nArgc);
typedef void (*respHandler)(const char *pcResp, bool *pbStatus);

typedef struct __sAtCmdHandle
{
    const char *pcCmd;
    cmdHandler CmdHdlr;
    respHandler RespHdlr;
    int  nArgsCount;
    char *pcArgs[5];
}_sAtCmdHandle;


bool InitUart(void);
//void SendCommand(const char *cmd, char *pcArgs[], int nArgc);
void ProcessResponse(const char *pcResp, bool *pbStatus);
bool ConfigureAndConnectWiFi();
//void ConnectToWiFi(const char *pcCmd, char *pcArgs[]);
bool IsWiFiConnected();
bool SendLocation();
//void SendLocation(const char *cmd);

#endif

//EOF