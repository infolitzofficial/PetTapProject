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
        printk("%c\n\r", (char)ucByte);
        switch(eUartRxState)
        {
            case START: if (ucByte == '*')
                        {
                            usRxBufferIdx = 0;
                            memset(cRxBuffer, 0, sizeof(cRxBuffer));
                            eUartRxState = RCV;
                        }
                        break;

            case RCV:   if (ucByte == '#')
                        {
                            
                            cRxBuffer[usRxBufferIdx++] = '\0';
                            bRxCmplt = true;
                            eUartRxState = START;
                            printk(".\n\r");
                            //k_msgq_put(&UartMsgQueue, &cRxBuffer, K_NO_WAIT);
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

        for (index = 0; index < usLength; index++)
        {
            printf("%c", (char)pcData[index]);
            uart_poll_out(psUartDev, (char)pcData[index]);
            k_usleep(50);
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

#if 0
/**
 * @brief Callback for LoRa Work
 * @param psLoRaWork : LoRa Work
 * @return None
*/
void LoRaE32RxWorkCb(void)
{
    bool bRetVal = false;
    char cBuffer[SND_BUFFER] = {0};
    _sLoRaRqstHandler sLoRaRqstHandler = {0};
    uint8_t uLoRaRxPacket[SND_BUFFER] = {0};
    // _sMeshRoleHandler *sMeshRoleHndler = NULL;
    char *pcBuffer = NULL;
    // _eNetworkRole *eNetworkRole = NULL;

    bRetVal = ReadLoRaPacket((uint8_t *)cBuffer);
    // sMeshRoleHndler = GetMeshRoleHandler();
    // eNetworkRole = GetNetWorkRole();

    if (bRetVal)
    {
        if (strlen(cBuffer) <= 4)
        {
            printk("INFO : Received LoRa Request\n\r");
            memcpy(&sLoRaRqstHandler, cBuffer, sizeof(_sLoRaRqstHandler));
            printk("INFO : Received LoRa Rqst Type : %d\n\r", sLoRaRqstHandler.ucRqstType);
            printk("INFO : Received LoRa Rqst Hndl : %d\n\r", sLoRaRqstHandler.usRqstHndl);

            if (sLoRaRqstHandler.ucRqstType == 0x02)
            {
                //
            }
            
            if (sLoRaRqstHandler.ucRqstType == 0x05)
            {
                // bLiveNotifyFromLoRA = true;
                // for (size_t i = 0; i < 1; i++)
                {
                    // k_msleep(1000);
                    pcBuffer = generateUniqueString(SND_BUFFER);
                    printk("INFO : Generated Unique String : %s\n\r", pcBuffer);
                    uint16_t usLength = AddMakers(ucLiveNotifyData, pcBuffer, strlen(pcBuffer));
                    SendData((uint8_t *)ucLiveNotifyData, strlen(ucLiveNotifyData));
                    free(pcBuffer);
                    
                }
                
            }

            if (sLoRaRqstHandler.ucRqstType == 0x09)
            {
                bLiveNotifyFromLoRA = false;
            }
            // switch(sLoRaRqstHandler.ucRqstType)
            // {
            //     case 0x02:
            //         {
            //             *eNetworkRole = LORA_SUPERVISOR;
            //         }
            //         break;
            //     case NOTIFICATION_LIVE:
            //         {
            //             bLiveNotifyFromLoRA = true;
            //         }
            //         break;
            //     case LIVE_NOTIFICATION_DISABLE:
            //         {
            //             bLiveNotifyFromLoRA = false;
            //         }
            //         break;
            //     default:
            //         break;
            // }
            
            
        }
        else
        {
            printk("INFO : LoRa Packet Received is %s\n\r, with length %d", cBuffer, strlen(cBuffer));
        }

        bRxCmplt = false;
        
    }
}

void sLoRaTxWrkHandlerCb(struct k_work *sLoRaTxWorkHandler)
{
    bool bRetVal = false;
    _sLoRaRqstHandler sLoRaRqstHandler = {0};
    // _sMeshRoleHandler *sMeshRoleHndler = NULL;
    uint8_t ucBuffer[10] = {0};
    uint16_t usLength = 0;

    // sMeshRoleHndler = GetMeshRoleHandler();

    switch (eLoRaState)
    {

        case ROLE_SUPERVISOR:
            {
                if (1)
                {
                    sLoRaRqstHandler.ucRqstType = 0x02;
                    sLoRaRqstHandler.usRqstHndl = 0x0011;
                    usLength = AddMakers(ucBuffer, &sLoRaRqstHandler, sizeof(_sLoRaRqstHandler));

                    SendData(ucBuffer, usLength);
                    printk("INFO : Sent LoRa Rqst Type : %d\n\r", sLoRaRqstHandler.ucRqstType);
                }
                
            }
            break;
        case NOTIFICATION_LIVE:
            {
                if (1)
                {
                    sLoRaRqstHandler.ucRqstType = 0x05;
                    sLoRaRqstHandler.usRqstHndl = 56;
                    usLength = AddMakers(ucBuffer, &sLoRaRqstHandler, sizeof(_sLoRaRqstHandler));

                    SendData(ucBuffer, usLength);
                    printk("INFO : Sent LoRa Rqst Type : %d\n\r", sLoRaRqstHandler.ucRqstType);
                }
            }
            break;
        case 9:
            {
                if (1)
                {
                    sLoRaRqstHandler.ucRqstType = 0x09;
                    sLoRaRqstHandler.usRqstHndl = 0x0013;
                    usLength = AddMakers(ucBuffer, &sLoRaRqstHandler, sizeof(_sLoRaRqstHandler));

                    SendData(ucBuffer, usLength);
                    printk("INFO : Sent LoRa Rqst Type : %d\n\r", sLoRaRqstHandler.ucRqstType);
                }
            }
            break;
    }
}

// bool CheckLoRaRqst(_sLoRaRqstHandler *psLoRaRqstHandler)
// {
//     bool bRetVal = false;
//     _eNetworkRole *peNetworkRole = NULL;

//     peNetworkRole = GetNetWorkRole();

//     switch (psLoRaRqstHandler->ucRqstType)
//     {
//         case 2:
//             *peNetworkRole = 0x02;
//             break;
//         case NOTIFICATION_LIVE:
//             *peNetworkRole = NOTIFICATION_LIVE;
//             break;
//         default:
//             break;
//     }

// }
/**
 * @brief Get LoRa Work Handler
 * @return LoRa Work Handler
*/
_sLoRaWrkHandler *GetLoRaWrkHndlr(void)
{
    return &sLoRaWrkHndlr;
}

void SetLoRaRole(_eNetworkRole eNetworkRole)
{
    eLoRaState = eNetworkRole;
}

uint16_t AddMakers(uint8_t *pcData, void *pData, uint16_t usLength)
{
    uint8_t ucBuffer[SND_BUFFER + 5] = {0};

    ucBuffer[0] = '*';
    memcpy(ucBuffer + 1, (uint8_t *)pData, usLength);
    ucBuffer[usLength + 1] = '#';
    memcpy(pcData, ucBuffer, usLength + 2);
    return usLength + 2;
}

bool GetLiveNotifyStatusFromLoRa(uint8_t *pcBuffer)
{
    // memcpy(ucLiveNotifyData, pcBuffer, strlen(pcBuffer));
    return bLiveNotifyFromLoRA;
}

char* generateUniqueString(uint16_t length)
{
    char* charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+{}[]|;:,.<>?";
    char* str = (char*)malloc((length + 1) * sizeof(char)); // Allocate memory for the string
    if (str == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    srand(time(NULL)); // Seed for randomization
    for (int i = 0; i < length; ++i) {
        int index = rand() % (int)(sizeof(charset) - 1);
        str[i] = charset[index];
    }
    str[length] = '\0'; // Null-terminate the string
    return str;
}


void StartLoRaTask()
{
    uint8_t ucBuffer[MSG_SIZE] = {0};
    char *pcBuffer = NULL;
    uint8_t ucWriteBuf[110] = {0};
    while (true)
    {
        
        k_msgq_get(&UartMsgQueue, ucBuffer, K_FOREVER);
        if (strlen(ucBuffer) <= 4)
        {
            printk("Request Received \n\r");
            memcpy(&sLoRaRqstHandler, ucBuffer, sizeof(_sLoRaRqstHandler));
            printk("INFO : Received LoRa Rqst Type : %d\n\r", sLoRaRqstHandler.ucRqstType);
            printk("INFO : Received LoRa Rqst Hndl : %d\n\r", sLoRaRqstHandler.usRqstHndl);

            

            if (sLoRaRqstHandler.ucRqstType == NOTIFICATION_LIVE)
            {
                for (size_t i = 0; i < 1; i++)
                {
                    pcBuffer = generateUniqueString(sLoRaRqstHandler.usRqstHndl);
                    if (pcBuffer)
                    {
                        printk("INFO : Generated Unique String : %s\n\r", pcBuffer);
                        uint16_t usLength = AddMakers(ucLiveNotifyData, pcBuffer, strlen(pcBuffer));
                        SendData((uint8_t *)ucLiveNotifyData, strlen(ucLiveNotifyData));
                        free(pcBuffer);                
                    }
                    // k_msleep(2000);
                }
                

            }
            
        }
        else 
        {
            printk("Received buffer is %s \n With the length of %d\n\r", ucBuffer, strlen(ucBuffer));
        }
    }
    
}

#endif
