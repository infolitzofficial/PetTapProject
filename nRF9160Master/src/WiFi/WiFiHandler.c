/**
 * @file    : WiFiHandler.c
 * @brief   : File contains WiFi related functions
 * @author  : Adhil
 * @date    : 06-03-2024
 * @note
*/

/*******************************************INCLUDES********************************************************/
#include "WiFiHandler.h"
#include "../System/SystemHandler.h"
#include "../PacketHandler/PacketHandler.h"
#include <string.h>
#include <sys/_stdint.h>
#include "../NVS/NvsHandler.h"
#include "zephyr/sys/printk.h"
#include "../BMS/BMHandler.h"

/*******************************************MACROS*********************************************************/
#define MSG_SIZE 255
#define WIFI_SSID_PWD       "realme GT 5G,s3qqyipp" //Change this line with SSID and password of choice
//#define AWS_BROKER		"a1kzdt4nun8bnh-ats.iot.ap-northeast-2.amazonaws.com"
#define AWS_BROKER          "a3kbzziaf9hg4n-ats.iot.us-east-1.amazonaws.com"
#define AWS_THING 		    "test_aws_iot"
#define AWS_TOPIC 		    "test_aws_iot/testtopic"
#define CFG_NUM 	        1
#define CFG_NAME 	        "latlong"
#define RETRY_COUNT         2

char cWifiCredentials[80] = "Alcodex,Adx@2013"; //SSID and password



/******************************************GLOBALS VARIABLES**********************************************/
static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));
bool bResponse = false;         //For check response received for AT command
/*Buffer for UART receive data*/
static uint8_t cRxBuffer[MSG_SIZE] = {0};
/*State of UART receive*/
static _eWiFiUartRxState eWiFiUartRxState = UART_START;
/*Index of Receiving buffer*/
static uint16_t usRxBufferIdx = 0;
/*Flag for packet receive completion*/
static bool bRxCmplt = false;
static char cSsid[20] = {0};
static char cPwd[20] = {0};

K_MSGQ_DEFINE(UartMsgQueue, MSG_SIZE, 10, 4);
/*****************************************PRIVATE FUNCTIONS***********************************************/
static void ProcessConnectionStatus(const char *pcResp, bool *pbStatus);
static void CheckConnection(const char *pcResp, bool *pbStatus);
static void SendCmdWithArgs(const char *cmd, char *pcArgs[], int nArgc);
static void SendCommand(const char *cmd, char *pcArgs[], int nArgc);

//Table of AT Commands and their handlers
_sAtCmdHandle sAtCmdHandle[] = {
    //CMD                                           //Handler       //RespHandler     //argument cnt    //Arguments
    {"AT\n\r",                                      SendCommand,     ProcessResponse,       0,            {NULL}                    },
    {"AT+WFMODE=0\n\r",                             SendCommand,     ProcessResponse,       0,            {NULL}                    },
    {"AT+WFJAPA=%s\n\r",                            SendCmdWithArgs, ProcessResponse,       1,            {cWifiCredentials, NULL,NULL}},
    {"AT+AWS=SET APP_PUBTOPIC %s\r\n",              SendCmdWithArgs, ProcessResponse,       1,            {AWS_TOPIC, NULL, NULL}   },
    {"AT+AWS=CFG  0 latshad 1 1\r\n",               SendCommand,     ProcessResponse,       0,            {NULL}                    },
    {"AT+AWS=CMD MCU_DATA 0 latshad init\r\n",      SendCommand,     ProcessResponse,       0,            {NULL}                    },
    {"AT+AWS=CFG %d %s 1 0\r\n",                    SendCmdWithArgs, ProcessResponse,       2,            {CFG_NUM, CFG_NAME, NULL} },
};  




/******************************************FUNCTION DEFINITIONS******************************************/  
/**
 * @brief Read data from Rx buffer
 * @param None
 * @return true for success
*/
bool ReadBuff(void)
{
    uint8_t ucByte = 0;
    bool bRetval = false;
 
    if (uart_fifo_read(uart_dev, &ucByte, 1) == 1)
    {
        switch(eWiFiUartRxState)
        {
            case UART_START: if (ucByte != '\n' && ucByte != '\r')
                        {
                            usRxBufferIdx = 0;
                            memset(cRxBuffer, 0, sizeof(cRxBuffer));
                            cRxBuffer[usRxBufferIdx++] = ucByte;
                            eWiFiUartRxState = UART_RCV;
                        }
                        break;
 
            case UART_RCV:   if (ucByte == '\n')
                        {
                           
                            cRxBuffer[usRxBufferIdx++] = '\0';
                            bRxCmplt = true;
                            eWiFiUartRxState = UART_START;
                            k_msgq_put(&UartMsgQueue, &cRxBuffer, K_NO_WAIT);
                        }
                        cRxBuffer[usRxBufferIdx++] = ucByte;
                        break;
 
            case UART_END:   eWiFiUartRxState = UART_START;
                        usRxBufferIdx = 0;
                        break;
 
            default:    break;            
        }
        bRetval = true;
    }
 
    return bRetval;
}
/**
 * @brief       : Callback function for UART reception
 * @param [in]  : dev - UART handle 
 * @param [out] : user_data - arguments to UART callback
 * @return      : None
*/
void serial_cb(const struct device *dev, void *user_data)
{
    uint8_t c;

    if (!uart_irq_update(uart_dev))
    {
        printk("UART IRQ update failed\n\r");
        return;
    }

    if (!uart_irq_rx_ready(uart_dev))
    {
        printk("UART IRQ RX not ready\n\r");
        return;
    }

    if (!ReadBuff())
    {
        printk("UART reception failed\n\r");
        return;
    }
}

/**
 * @brief       : UART send function
 * @param [in]  : buf - Holds data to send over UART 
 * @param [out] : None
 * @return      : None
*/
void print_uart(const char *buf)
{
    int msg_len = strlen(buf);

    for (int i = 0; i < msg_len; i++)
    {
        uart_poll_out(uart_dev, buf[i]);
    }
}


/**
 * @brief       : Callback function for sending AT command
 * @param [in]  : cmd - AT command 
 *              : nArgc - argument count
 * @param [out] : pcArgs - arguments to AT command
 * @return      : None
*/
static void SendCommand(const char *cmd, char *pcArgs[], int nArgc)
{
    print_uart(cmd);
    k_msleep(500); 
}

/**
 * @brief       : Configure WiFi. Configuration
 *                includes AWS configurations also
 * @param [in]  : None
 * @param [out] : None
 * @return      : true for success
*/
bool ConfigureWiFi()
{
    bool bRetVal = false;
    uint8_t ucIdx = 0;
    uint32_t TimeNow=0;
    char cRespBuff[255] = {0};
    int8_t nRetry = 0;

    for (ucIdx = 0; ucIdx < sizeof(sAtCmdHandle)/sizeof(sAtCmdHandle[0]); ucIdx++)
    {
        if (sAtCmdHandle[ucIdx].pcCmd)
        {
            //no op
        }
        else
        {
            printk("Invalid Command\n\r");
            break;
        }
        
        nRetry = RETRY_COUNT;

        do
        {
            sAtCmdHandle[ucIdx].CmdHdlr(sAtCmdHandle[ucIdx].pcCmd, sAtCmdHandle[ucIdx].pcArgs, sAtCmdHandle[ucIdx].nArgsCount);
            printk("Sending: %s\n\r", sAtCmdHandle[ucIdx].pcCmd);
            k_msleep(100);

            if (0 == k_msgq_get(&UartMsgQueue, cRespBuff, K_MSEC(100)))
            {
                printk("Response: %s\n\r", cRespBuff);
                k_msleep(100);
                sAtCmdHandle[ucIdx].RespHdlr(cRespBuff, &bResponse);
                if (bResponse)
                {
                    printk("OK: cmd%s", sAtCmdHandle[ucIdx].pcCmd);
                    k_msleep(100);
                    bRetVal = true;
                    goto Cmplt;
                }
            }

        }while((nRetry--) > 0);

        if (!bRetVal)
        {
            break;
        }

        Cmplt: //No Operation

    }

    return bRetVal;

}

/**
 * @brief       : Check whether configured AP is disconnected
 * @param [in]  : pcResp - Response from DA16200
 * @param [out] : pbStatus - Status 
 * @return      : None
*/
static void CheckAPDisconnected(const char *pcResp, bool *pbStatus)
{
    if (strstr(pcResp, "+WFDAP:0") != NULL)
    {
        *pbStatus = true;
    }
    else
    {
        *pbStatus = false;
    } 
}

/**
 * @brief       : Check whether configured AP is connected
 * @param [in]  : pcResp - Response from DA16200
 * @param [out] : pbStatus - Status 
 * @return      : None
*/
static void CheckAPConnected(const char *pcResp, bool *pbStatus)
{
    #ifdef NVS_ENABLE
    int8_t uCredentialIdx = 0;
    _sConfigData *psConfigData = NULL;

    psConfigData = GetConfigData();
#endif
    if (strstr(pcResp, "+WFJAP:1") != NULL)
    {
        *pbStatus = true;
        if (strstr(pcResp, cSsid) != NULL) 
        {
#ifdef NVS_ENABLE
        for (uCredentialIdx = 0;uCredentialIdx < 5; uCredentialIdx++) 
        {

            if (psConfigData[uCredentialIdx].bCredAddStatus == true) 
            {
                psConfigData[uCredentialIdx].bWifiStatus = false;
            }
            else 
            {
                if (strstr(pcResp, cSsid) != NULL) 
                {
                    printk("Writting SSIS : %s\n", cSsid);
                    printk("Writting SSIS : %s\n", cSsid);
                    memcpy(&psConfigData[uCredentialIdx].sWifiCred.ucSsid, cSsid, strlen(cSsid));
                    memcpy(&psConfigData[uCredentialIdx].sWifiCred.ucPwd, cPwd, strlen(cPwd));
                    psConfigData[uCredentialIdx].bWifiStatus = true;
                    psConfigData[uCredentialIdx].bCredAddStatus = true;

                    memset(cSsid, 0, sizeof(cSsid));
                    memset(cPwd, 0, sizeof(cPwd));

                    WriteCredToFlash();
                }
                
                break;
            }

        }
        

#endif       
        }
    }
    else
    {
        *pbStatus = false;
    } 
}

/**
 * @brief      : GetAPCredentials
 * @param [in] : None
 * @param [out]: None
 * @return     : cWifiCredentials
*/
char *GetAPCredentials(void)
{
    return cWifiCredentials;
}

/**
 * @brief      : SetAPCredentials
 * @param [in] : pcCredential
 * @param [out]: None
 * @return     : None
*/
void SetAPCredentials(char *pcCredential)
{
    if (pcCredential != NULL)
    {
        memset(cWifiCredentials, 0, sizeof(cWifiCredentials));
        strcpy(cWifiCredentials, pcCredential);
    }
    else
    {
        printk("Error: pcCredential is NULL");
    }
}

/**
 * @brief       : Processs Msgs from Wifi
 * @param [in]  : None
 * @param [out] : None 
 * @return      : None
*/
bool ProcessWiFiMsgs( )
{
    char cRxBuffer[255];
    _eDevState *DevState = NULL;
    bool bStatus = false;

    DevState = GetDeviceState();
    if (0 == k_msgq_get(&UartMsgQueue, cRxBuffer, K_MSEC(100)))
    {
        printk("DEBUG: DA RESPONSE%s\n", cRxBuffer);
        CheckAPConnected(cRxBuffer, &bStatus);
        if (bStatus)
        {
            SetDeviceState(WIFI_CONNECTED); 
            printk("DEBUG: Device connected to %s\n", cRxBuffer);
            return bStatus;        
        }
        
        CheckAPDisconnected(cRxBuffer, &bStatus);

        if (bStatus)
        {
            SetDeviceState(WIFI_DISCONNECTED);
            printk("DEBUG: Device disconnected connected to %s\n", cRxBuffer);
            return bStatus;
        }
    }

}

/**
 * @brief       : Process AT command response from WiFi module
 * @param [in]  : pcResp - AT command response
 * @param [out] : pbStatus - AT command stauts true for success else failed
 * @return      : None
*/
void ProcessResponse(const char *pcResp, bool *pbStatus)
{
    if (strstr(pcResp, "OK") != NULL)
    {
        *pbStatus = true;
    }
    else
    {
        *pbStatus = false;
    }  
}

/**
 * @brief       : Checking connection status from WiFi
 * @param [in]  : pcResp - AT command response
 * @param [out] : pbStatus - AT command stauts true for success else failed
 * @return      : None
*/
static void ProcessConnectionStatus(const char *pcResp, bool *pbStatus)
{

    if (strstr(pcResp, "+WFSTA:1") != NULL)
    {
        *pbStatus = true;

    }
    else
    {
        *pbStatus = false;
    }  
}

/**
 * @brief       : Initialise UART channel for WIFI interfacing
 * @param [in]  : None
 * @param [out] : None
 * @return      : true for success
*/
bool InitUart(void)
{
    int nRetVal = 0;
    bool bRetVal = false;

    do 
    {
        if (!device_is_ready(uart_dev)) 
        {
            printk("UART device not found!");
            break;
        }

        nRetVal = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);

        if (nRetVal < 0) 
        {
            if (nRetVal == -ENOTSUP) 
            {
                printk("Interrupt-driven UART API support not enabled\n");
                break;
            } 
            else if (nRetVal == -ENOSYS) 
            {
                printk("UART device does not support interrupt-driven API\n");
                break;
            } 
            else 
            {
                printk("Error setting UART callback: %d\n", nRetVal);
                break;
            }
        }

        uart_irq_rx_enable(uart_dev);
        printk("UART initialised\n\r");
        bRetVal = true;
    } while(0);

    return bRetVal;
}

/**
 * @brief       : Check if WiFi is connected 
 * @param [in]  : None
 * @param [out] : None
 * @return      : true for success
*/
bool IsWiFiConnected()
{
    bool bRetVal = false;
    bool bResponse = false;
    char cCmdBuff[255];

    bRxCmplt = false;
    strcpy(cCmdBuff, "AT+WFSTA\n\r");
    print_uart(cCmdBuff);

    k_msgq_get(&UartMsgQueue, cCmdBuff, K_MSEC(100));
    printk("ConnResponse: %s\n\r", cCmdBuff);
    ProcessConnectionStatus(cCmdBuff, &bResponse);
    if (bResponse)
    {
        bRetVal = true;
    }

    return bRetVal;
}

/**
 * @brief       : Disconnect DA module
 * @param [in]  : None
 * @param [out] : None
 * @return      : true for success
*/
bool DisconnectFromWiFi()
{
    
    bool bRetVal = false;
    bool bResponse = false;
    char cCmdBuff[255];

    bRxCmplt = false;
    strcpy(cCmdBuff, "AT+WFQAP\n\r");
    print_uart(cCmdBuff);

    k_msgq_get(&UartMsgQueue, cCmdBuff, K_MSEC(100));
    printk("Disconnect Cmd Response: %s\n\r", cCmdBuff);
    ProcessResponse(cCmdBuff, &bResponse);

    if (bResponse)
    {
        bRetVal = true;
    }

    return bRetVal;
}

/**
 * @brief       : Callback for sending command with arguments
 * @param [in]  : cmd - AT command
 *                nArgc - argument count
 * @param [out] : pcArgs - arguments to the callback
 * @return      : None
*/
static void SendCmdWithArgs(const char *cmd, char *pcArgs[], int nArgc)
{
    char cmdBuf[255];

    switch(nArgc)
    {
        case 0: printk("ERR: Invalid args\n\r");
                k_msleep(500);
                break;

        case 1:
                sprintf(cmdBuf, cmd, pcArgs[0]); 
                break;

        case 2: 
                sprintf(cmdBuf, cmd, pcArgs[0], pcArgs[1]);
                break;

        case 3:
                sprintf(cmdBuf, cmd, pcArgs[0], pcArgs[1], pcArgs[2]);
                break;

        default:
                break; 

    }

    print_uart(cmdBuf);
    k_msleep(500); // Adjust the delay based on the module's response time
}

/**
 * @brief       : function for sending location data over WiFi
 * @param [in]  : None
 * @param [out] : None
 * @return      : true for success
*/
bool SendPayload()
{
    _sGnssConfig *psLocationData = NULL;
    bool bRetVal = false;
    char cPayload[50]; //Location data buffer
    char cATcmd[100]; //AT command buffer 
    float fTempCharger = 0.0; 
    float fVoltcharger=0.00;
    fVoltcharger= ReadI2CVoltage();
    fTempCharger= ReadI2CTemperature();
    psLocationData = GetLocationData();
    

    if (psLocationData)
    {
        sprintf(cPayload,"%.6f/%.6f/VC:%.2f/TC:%.2f", psLocationData->dLatitude, psLocationData->dLongitude, fVoltcharger, fTempCharger);
        printk("sending data: %s\n\r", cPayload);
        sprintf(cATcmd, "AT+AWS=CMD MCU_DATA %d %s %s\r\n", CFG_NUM, CFG_NAME, cPayload);
        print_uart(cATcmd);
        k_msleep(500);
        bRetVal = true;
    }

    return bRetVal;
}


_sAtCmdHandle *GetATCmdHandle()
{   
    return &sAtCmdHandle;
}


void SetWifiCred(char *pcSsid, char *pcPwd)
{
    strcpy(cSsid, pcSsid);
    strcpy(cPwd, pcPwd);
}

uint8_t CheckLastConnectedStatus(void)
{
#ifdef NVS_ENABLE
    int8_t uCredentialIdx = 0;
    _sConfigData *psConfigData = NULL;

    psConfigData = GetConfigData();

    // Loop through all credentials
    for (uCredentialIdx = 0; uCredentialIdx < 5; uCredentialIdx++)
    {
        // Check if the credential is added
        if (psConfigData[uCredentialIdx].bCredAddStatus == true)
        {
            // Check the last connected status
            if (psConfigData[uCredentialIdx].bWifiStatus == true)
            {
                // Return idex
                return uCredentialIdx;
            }
        }
    }
#endif

    // If not found, return false
    return false;
}


//EOF