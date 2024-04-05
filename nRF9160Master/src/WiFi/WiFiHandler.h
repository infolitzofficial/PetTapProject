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

typedef enum __eWiFiUartRxState
{
    UART_START,
    UART_RCV,
    UART_END
}_eWiFiUartRxState;
typedef struct __sAtCmdHandle
{
    const char *pcCmd;
    cmdHandler CmdHdlr;
    respHandler RespHdlr;
    int  nArgsCount;
    char *pcArgs[ARGS_CNT];
}_sAtCmdHandle;

typedef struct __attribute__((__packed__)) __sWifiCred
{
    uint8_t ucSsid[20];
    uint8_t ucPwd[20];
}_sWifiCred;

/***********************************************FUNCTION DECLARATIONS**************************************/
bool InitUart(void);
void ProcessResponse(const char *pcResp, bool *pbStatus);
bool ConfigureWiFi();
bool IsWiFiConnected();
bool ProcessWiFiMsgs();
bool SendLocation();
bool ReadBuff(void);
bool DisconnectFromWiFi();
char *GetAPCredentials(void);
void SetAPCredentials(char *pcCredential);
_sAtCmdHandle *GetATCmdHandle();
#endif

//EOF