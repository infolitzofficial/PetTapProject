/**
 * @file    : UartHandler.c
 * @brief   : Functions for handling uart peripheral
 * @author  : Adhil
 * @date    : 13-03-2024
*/
/*******************************************************INCLUDES***************************************************/

#include "UartHandler.h"
#include "BleService.h"

/*******************************************************MACROS*****************************************************/


/*******************************************************TYPEDEFS***************************************************/


/*******************************************************PRIVATE VARIABLES******************************************/
/*Get UART device*/
static const struct device *psUartDev = DEVICE_DT_GET(DT_NODELABEL(arduino_serial));
/*Buffer for UART receive data*/
static uint8_t cRxBuffer[BUFFER_SIZE] = {0};
/*State of UART receive*/
static _eUartRxState eUartRxState = START;
/*Index of LoRa packet receive*/
static uint16_t usRxBufferIdx = 0;
/*Flag for LORA packet receive completion*/
static bool bRxCmplt = false;

/*******************************************************PUBLIC VARIABLES*******************************************/



/*******************************************************FUNCTION DEFINITION*****************************************/

/**
 * @brief Initialising LoRa
 * @param None
 * @return true for success
*/
bool InitUart(void)
{
    int nRetVal = 0;
    bool bRetVal = false;
 
    do
    {
        if (!device_is_ready(psUartDev))
        {
            printk("UART device not found!");
            break;
        }

        nRetVal = uart_irq_callback_user_data_set(psUartDev, ReceptionCb, NULL);
 
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
 
        uart_irq_rx_enable(psUartDev);
        printk("UART initialised\n\r");

        bRetVal = true;
    } while(0);
 
    return bRetVal;
}
/**
 * @brief Callback for LoRa Receive Interrupt
 * @param dev - UART device
 * @param user_data - User data
 * @return None
*/
 
void ReceptionCb(const struct device *dev, void *user_data)
{
    if (psUartDev)
    {
        if (!uart_irq_update(psUartDev))
        {
            return;
        }
 
        if (!uart_irq_rx_ready(psUartDev))
        {
            return;
        }
 
        if (!ReadBuffer())
        {
            printk("Uart reception failed\n\r");
        }
    }
   
}
/**
 * @brief Read data from Rx buffer
 * @param None
 * @return true for success
*/
bool ReadBuffer(void)
{
    uint8_t ucByte = 0;
    bool bRetval = false;

    if (uart_fifo_read(psUartDev, &ucByte, 1) == 1)
    {
        switch(eUartRxState)
        {
            case START: if (ucByte == '*')
                        {
                            usRxBufferIdx = 0;
                            memset(cRxBuffer, 0, sizeof(cRxBuffer));
                            cRxBuffer[usRxBufferIdx++] = ucByte;
                            eUartRxState = RCV;
                        }
                        break;

            case RCV:   if (ucByte == '#')
                        {
                            cRxBuffer[usRxBufferIdx++] = ucByte;
                            cRxBuffer[usRxBufferIdx++] = '\0';
                            bRxCmplt = true;
                            eUartRxState = START;
                            printk(".\n\r");
                        }
                        cRxBuffer[usRxBufferIdx++] = ucByte;
                        break;

            case END:   eUartRxState = START;
                        usRxBufferIdx = 0;
                        break;

            default:    break;            
        }
        bRetval = true;
    }
 
    return bRetval;
}
/**
 * @brief  Send data via uart
 * @param  pcData : data to send
 * @return None
*/
void SendData(const uint8_t *pcData, uint16_t usLength)
{
    uint16_t index = 0;

    if (pcData)
    {

        for (index = 0; index <= usLength; index++)
        {
            uart_poll_out(psUartDev, (char)pcData[index]);
            k_msleep(5);
        }
        printk("\n\r");
    }
}

/**
 * @brief Read LoRa packet
 * @param pucBuffer : LoRa packet buffer
 * @return true for success
*/
bool ReadPacket(uint8_t *pucBuffer)
{
    bool bRetVal = false;

    if (bRxCmplt)
    {
        memcpy(pucBuffer, cRxBuffer, usRxBufferIdx);
        bRxCmplt = false;
        bRetVal = true;
    }

    return bRetVal;
}

