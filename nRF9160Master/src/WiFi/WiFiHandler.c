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
#include <string.h>

/*******************************************MACROS*********************************************************/
#define MSG_SIZE 255
#define BUF_SIZE 255
#define WIFI_SSID_PWD       "realme GT 5G,s3qqyipp" //Change this line with SSID and password of choice
#define AWS_BROKER		    "a1kzdt4nun8bnh-ats.iot.ap-northeast-2.amazonaws.com"
#define AWS_THING 		    "test_aws_iot"
#define AWS_TOPIC 		    "test_aws_iot/testtopic"
#define CFG_NUM 	        1
#define CFG_NAME 	        "latlong"

/******************************************GLOBALS VARIABLES**********************************************/
static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;
static bool bRcvdData = false;  //For check received data
bool bResponse = false;         //For check response received

/*****************************************PRIVATE FUNCTIONS***********************************************/
static void ProcessConnectionStataus(const char *pcResp, bool *pbStatus);
static void SendCmdWithArgs(const char *cmd, char *pcArgs[], int nArgc);
static void SendCommand(const char *cmd, char *pcArgs[], int nArgc);

//Table of AT Commands and their handlers
_sAtCmdHandle sAtCmdHandle[] = {
    //CMD                                           //Handler       //RespHandler     //argument cnt    //Arguments
    {"AT\n\r",                                      SendCommand,     ProcessResponse,       0,            {NULL}                    },
    {"AT+WFMODE=0\n\r",                             SendCommand,     ProcessResponse,       0,            {NULL}                    },
    {"AT+WFJAPA=%s\n\r",                            SendCmdWithArgs, ProcessResponse,       1,            {WIFI_SSID_PWD, NULL,NULL}},
    {"AT+AWS=SET APP_PUBTOPIC %s\r\n",              SendCmdWithArgs, ProcessResponse,       1,            {AWS_TOPIC, NULL, NULL}   },
    {"AT+AWS=CFG  0 latshad 1 1\r\n",               SendCommand,     ProcessResponse,       0,            {NULL}                    },
    {"AT+AWS=CMD MCU_DATA 0 latshad init\r\n",      SendCommand,     ProcessResponse,       0,            {NULL}                    },
    {"AT+AWS=CFG %d %s 1 0\r\n",                    SendCmdWithArgs, ProcessResponse,       2,            {CFG_NUM, CFG_NAME, NULL} },
};  


/******************************************FUNCTION DEFINITIONS******************************************/
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

    while (uart_fifo_read(uart_dev, &c, 1) == 1)
    {
        if ((c == '\r') && rx_buf_pos > 0)
        {
            rx_buf[rx_buf_pos] = '\0';
            bRcvdData = true;
            rx_buf_pos = 0;
        }
        else if (rx_buf_pos < (sizeof(rx_buf) - 1))
        {
            rx_buf[rx_buf_pos++] = c;
        }
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
 * @brief       : Configure and attempt connection to WiFi. Configuration
 *                includes AWS configurations also
 * @param [in]  : None
 * @param [out] : None
 * @return      : true for success
*/
bool ConfigureAndConnectWiFi()
{
    bool bRetVal = false;
    uint8_t ucIdx = 0;
    uint32_t TimeNow=0;
    int8_t nRetry =3;

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
        
        nRetry = 2;

      //  do
      //  {
            do
            {
                bRcvdData = false;
                bResponse = false;
                sAtCmdHandle[ucIdx].CmdHdlr(sAtCmdHandle[ucIdx].pcCmd, sAtCmdHandle[ucIdx].pcArgs, sAtCmdHandle[ucIdx].nArgsCount);
                printk("Sending: %s\n\r", sAtCmdHandle[ucIdx].pcCmd);
                TimeNow = sys_clock_tick_get();

                while (sys_clock_tick_get() - TimeNow < (TICK_RATE * 15))
                {
                    if (bRcvdData)
                    {
                        printk("Response: %s\n\r", rx_buf);
                        sAtCmdHandle[ucIdx].RespHdlr(rx_buf, &bResponse);
                        if (bResponse)
                        {
                            printk("OK: cmd%s", sAtCmdHandle[ucIdx].pcCmd);
                            bRetVal = true;
                            goto Cmplt;
                        }
                    }
                }

            }while((nRetry--) > 0);

            if (!bRetVal)
            {
                break;
            }
        Cmplt://NoP
       // }
        //} while(bRcvdData == false);

    }

    return bRetVal;

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
static void ProcessConnectionStataus(const char *pcResp, bool *pbStatus)
{

    if (strstr(pcResp, "WFSTA:1") != NULL)
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
    char cCmdBuff[50];

    bRcvdData = false;
    strcpy(cCmdBuff, "AT+WFSTA\n\r");
    print_uart(cCmdBuff);

    if (bRcvdData)
    {
        printk("Response: %s\n\r", rx_buf);
        ProcessConnectionStataus(rx_buf, &bResponse);
        if (bResponse)
        {
            bRetVal = true;
        }
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
bool SendLocation()
{
    _sGnssConfig *psLocationData = NULL;
    bool bRetVal = false;
    char cPayload[50];
    char cATcmd[100];

    psLocationData = GetLocationData();

    if (psLocationData)
    {
        sprintf(cPayload,"%.6f,%.6f", psLocationData->dLatitude, psLocationData->dLongitude);
        printk("sending data: %s\n\r", cPayload);
        sprintf(cATcmd, "AT+AWS=CMD MCU_DATA %d %s %s\r\n", CFG_NUM, CFG_NAME, cPayload);
        print_uart(cATcmd);
        k_msleep(500);
        bRetVal = true;
    }

    return bRetVal;
}