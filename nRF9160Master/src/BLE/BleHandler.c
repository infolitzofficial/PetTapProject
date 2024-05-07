/**
 * @file   : BleHandler.c
 * @brief  : Files handling 52840 related functions
 * @author : Adhil
 * @date   : 05-03-2024
 * @ref    : BleHandler.h
*/

/*******************************************INCLUDES********************************************************/
#include "../System/SystemHandler.h"
#include "../PacketHandler/PacketHandler.h"
#include "BleHandler.h"
#include "zephyr/sys/printk.h"
#include "../BMS/BMHandler.h"
#include "../MC3630/AccelerometerHandler.h"

/*******************************************MACROS**********************************************************/
#define MSG_SIZE        255
#define PAYLOAD_SIZE    75

/******************************************TYPEDEFS*********************************************************/

/******************************************PRIVATE GLOBALS**************************************************/
static const struct device *BleUart = DEVICE_DT_GET(DT_NODELABEL(uart2));
/*Buffer for UART receive data*/
static uint8_t cRxBuffer[BUFFER_SIZE] = {0};
/*State of UART receive*/
static _eUartRxState eUartRxState = START;
/*Index of LoRa packet receive*/
static uint16_t usRxBufferIdx = 0;
/*Flag for LORA packet receive completion*/
static bool bRxCmplt = false;

K_MSGQ_DEFINE(BleMsgQueue, MSG_SIZE, 10, 4);
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

    if (!ReadBuffer())
    {
        printk("Uart reception failed\n\r");
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

    if (uart_fifo_read(BleUart, &ucByte, 1) == 1)
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
                            k_msgq_put(&BleMsgQueue, &cRxBuffer, K_NO_WAIT);
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
    printk("MsgLen: %d\n\r", usLen);

    for (int i = 0; i <= usLen; i++)
    {
        uart_poll_out(BleUart, pucBuff[i]);
        k_msleep(5); //delay 5ms
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

/**
 * @brief       : Read BLE packet
 * @param [in]  : None
 * @param [out] : pucBuffer : BLE packet buffer
 * @return      : true for success
*/
bool ReadPacket(uint8_t *pucBuffer)
{
    bool bRetVal = false;
    if (0 == k_msgq_get(&BleMsgQueue, pucBuffer, K_MSEC(100)))
    {
        printk("Data Received\n\r");
        bRetVal = true;
    }

    return bRetVal;
}

/**
 * @brief       : Send location data to BLE
 * @param [in]  : None
 * @param [out] : None
 * @return      : true for success
*/
bool SendPayloadToBle()
{
    _sGnssConfig *psLocationData = NULL;
    bool bRetVal = false;
    char cPayload[PAYLOAD_SIZE];
    _sPacket sPacket = {0};
    float fTempCharger = 0.0; 
    float fVoltcharger=0.00;
    int iPetmove=0;
    MC36XX_acc_t PreAccRaw;

    memcpy(&PreAccRaw, GetMC36Data(), sizeof(MC36XX_acc_t));
    fVoltcharger= ReadI2CVoltage();
    fTempCharger= ReadI2CTemperature();
    psLocationData = GetLocationData();
    iPetmove=PetMove(PreAccRaw);

    if (psLocationData)
    {
        sprintf(cPayload,"%.6f/%.6f/VC:%.2f/TC:%.2f/PetMov:%d", psLocationData->dLatitude, psLocationData->dLongitude, fVoltcharger, fTempCharger,iPetmove);
        printk("sending data: %s\n\r", cPayload);
        BuildPacket(&sPacket, RESP, (uint8_t *)cPayload, strlen(cPayload));
        SendBleMsg(&sPacket, sizeof(_sPacket));
        bRetVal = true;
    }

    return bRetVal;
}

//EOF