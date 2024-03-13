/**
 * @file   : SystemHandler.c
 * @brief  : Files handling 52840 related functions
 * @author : Adhil
 * @date   : 05-03-2024
 * @ref    : SystemHandler.h
*/

/*******************************************INCLUDES********************************************************/
#include "../System/SystemHandler.h"
#include "BleHandler.h"

/*******************************************MACROS**********************************************************/
#define MSG_SIZE 1024

/******************************************TYPEDEFS*********************************************************/

/******************************************PRIVATE GLOBALS**************************************************/
static const struct device *BleUart = DEVICE_DT_GET(DT_NODELABEL(uart2));
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;
static bool bRcvdData = false;  //For check received data

/*****************************************FUNCTION DEFINITION***********************************************/
/**
 * @brief       : Callback function for UART reception
 * @param [in]  : dev - UART handle 
 * @param [out] : user_data - arguments to UART callback
 * @return      : None
*/
void BleReceptionCb(const struct device *dev, void *user_data)
{
    uint8_t c;

    if (!uart_irq_update(BleUart))
    {
        printk("UART IRQ update failed\n\r");
        return;
    }

    if (!uart_irq_rx_ready(BleUart))
    {
        printk("UART IRQ RX not ready\n\r");
        return;
    }

    while (uart_fifo_read(BleUart, &c, 1) == 1)
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
 * @brief       : Initialise UART channel for WIFI interfacing
 * @param [in]  : None
 * @param [out] : None
 * @return      : true for success
*/
bool InitBleUart(void)
{
    int nRetVal = 0;
    bool bRetVal = false;

    do 
    {
        if (!device_is_ready(BleUart)) 
        {
            printk("UART device not found!");
            break;
        }

        nRetVal = uart_irq_callback_user_data_set(BleUart, BleReceptionCb, NULL);

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

        uart_irq_rx_enable(BleUart);
        printk("UART initialised\n\r");
        bRetVal = true;
    } while(0);

    return bRetVal;
}

/**
 * @brief       : UART send function
 * @param [in]  : buf - Holds data to send over UART 
 * @param [out] : None
 * @return      : None
*/
void SendBleMsg(uint8_t *pucBuff, uint16_t usLen)
{
    //int msg_len = strlen(buf);

    for (int i = 0; i < usLen; i++)
    {
        uart_poll_out(BleUart, pucBuff[i]);
    }
}

/**
 * @brief       : Process AT command response from WiFi module
 * @param [in]  : pcResp - AT command response
 * @param [out] : pbStatus - AT command stauts true for success else failed
 * @return      : None
*/
void ProcessBleResponse(const char *pcResp, bool *pbStatus)
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