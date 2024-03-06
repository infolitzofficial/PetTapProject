/**
 * @file WiFiHandler.c
 * @brief File contains WiFi related functions
 * @author Jeslin
 * @date 15-01-2024
 * @note
*/

/**********************************************************INCLUDES*****************************************/
#include "WiFiHandler.h"
#include <string.h>
/****************************************MACROS************************************************************/
#define MSG_SIZE 255
#define BUF_SIZE 255
#define WIFI_SSID_PWD           "Alcodex,Adx@2013"

/******************************************GLOBALS VARIABLES**********************************************/
static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;
static bool bRcvdData = false;  //For check received data
bool bResponse = false;         //For check response received

/*****************************************PRIVATE FUNCTIONS***********************************************/
static void ProcessConnectionStataus(const char *pcResp, bool *pbStatus);

//Table of AT Commands and their handlers
_sAtCmdHandle sAtCmdHandle[] = {
    //CMD                                           //Handler       //RespHandler        //Arguments
    {"AT\n\r",                                       SendCommand,     ProcessResponse,   NULL},
    {"AT+WFMODE=0\n\r",                              SendCommand,     ProcessResponse,   NULL },
    {"AT+WFJAPA=%s\n\r",                             ConnectToWiFi,   ProcessResponse,   NULL}
};  


/******************************************FUNCTION DEFINITIONS******************************************/
/**
 * @brief Callback for UART Receive Interrupt
 * @param dev - UART device
 * @param user_data - User data
 * @return None
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
 * @brief Print message on UART
 * @param buf - Message
 * @return None
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
 * @brief Call back function for send command to the module
 * @param cmd - Command
 * @param pcArgs - Arguments
 * @return None
*/

void SendCommand(const char *cmd, const char *pcArgs[])
{
    char cCmdBuf[255];
    char cMsgCmd[255];
    uint8_t ucIdx = 0;
        
    print_uart(cmd);
    k_msleep(500); // Adjust the delay based on the module's response time

}

/**
 * @brief Sending message to the module
 * @param None
 * @return true for success
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
        
        nRetry = 3;

        do
        {
            do
            {
                bRcvdData = false;
                bResponse = false;
                sAtCmdHandle[ucIdx].CmdHdlr(sAtCmdHandle[ucIdx].pcCmd, sAtCmdHandle[ucIdx].pcArgs[0]);
                printk("Sending: %s\n\r", sAtCmdHandle[ucIdx].pcCmd);
                TimeNow = sys_clock_tick_get();

                while (sys_clock_tick_get() - TimeNow < (TICK_RATE * 4))
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
        Cmplt://NoP
        } while(bRcvdData == false);

    }

    return bRetVal;

}

/**
 * @brief Checking the response from the module
 * @param pcResp - Response from the module
 * @param pbStatus - Status of the response
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
  //  memset(rx_buf, 0, sizeof(rx_buf));
}

/**
 * @brief Checking the response from the module
 * @param pcResp - Response from the module
 * @param pbStatus - Status of the response
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

    //memset(rx_buf, 0, sizeof(rx_buf));
}
/**
 * @brief Initialising UART
 * @param None
 * @return true for success
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
 * @brief       :
 * @param [in]  :
 * @param [out] :
 * @return      :
*/
void ConnectToWiFi(const char *pcCmd, const char *pcArgs[])
{
    char cCmdBuff[50];
        
    sprintf(cCmdBuff, "AT+WFJAPA=%s\n\r", WIFI_SSID_PWD); //connect to wifi
    printk("INFO: Cmd: %s\n\r", cCmdBuff);
    print_uart(cCmdBuff);
    k_msleep(500);

}

/**
 * @brief       :
 * @param [in]  :
 * @param [out] :
 * @return      :
*/
bool IsWiFiConnected()
{
    bool bRetVal = false;
    bool bResponse = false;
    char cCmdBuff[50];

    bRcvdData = false;
    strcpy(cCmdBuff, "AT+WFSTA\n\r");
    print_uart(cCmdBuff);
    //k_msleep(2000);

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