/**
 * @file    : PacketHandler.c
 * @brief   : Functions for handling Packet mechanism
 * @author  : Adhil
 * @date    : 13-03-2024
*/
/*******************************************************INCLUDES***************************************************/
#include "PacketHandler.h"
#include "../WiFi/WiFiHandler.h"
#include "../BLE/BleHandler.h"
#include "../System/SystemHandler.h"
#include "zephyr/kernel.h"
#include "zephyr/sys/printk.h"
#include <ssp/string.h>
#include <string.h>
#include <sys/_stdint.h>


#include "../NVS/NvsHandler.h"


/*******************************************************MACROS*****************************************************/

/*******************************************************TYPEDEFS***************************************************/

static bool BleStatusFlag = false;

/*******************************************************PRIVATE VARIABLES******************************************/

/*******************************************************PUBLIC VARIABLES*******************************************/

/*******************************************************FUNCTION DEFINITION*****************************************/


/**
 * @brief      : Build Packet to send
 * @param [in] : usLen - Length of the packet
 *             : PcktType - PacketType
 * @param [out]: psPacket - Packet to build
 *             : pucPayload - Payload to send
 * @return     : returns true on success
*/
bool BuildPacket(_sPacket *psPacket,_ePacketType PcktType, 
                uint8_t *pucPayload, uint16_t usPayloadLen)
{
    bool bRetVal = false;
    
    // printk("Length of payload: %d\n\r", usPayloadLen);
    if (psPacket && pucPayload)
    {
        psPacket->ucStartByte = START_BYTE;
        psPacket->PacketType = PcktType;
        memcpy(psPacket->pucPayload, pucPayload, usPayloadLen);
        psPacket->usLen = usPayloadLen;
        psPacket->ucEndByte = END_BYTE;
        bRetVal = true;
    }

    return bRetVal;
}

/**
 * @brief      : Parse packet received
 * @param [in] : None
 * @param [out]: pucRcvdBuffer - Receieved Buffer
 *             : psPacket - Packet received
 * @return     : true for success
*/
bool ParsePacket(uint8_t *pucRcvdBuffer, _sPacket *psPacket)
{
    bool bRetVal = false;

    if (pucRcvdBuffer && psPacket)
    {
        memcpy(psPacket, pucRcvdBuffer, sizeof(_sPacket));
        bRetVal = true;
    }

    return bRetVal;
}

int SetBleStatus (bool flag)
{
    bool BleStatusFlag = flag;
}

bool GetBleStatus()
{
    return BleStatusFlag;
}


/**
 * @brief      : Parse packet received
 * @param [in] : None
 * @param [out]: pucRcvdBuffer - Receieved Buffer
 *             : psPacket - Packet received
 * @return     : true for success
*/
bool ProcessRcvdPacket(_sPacket *psPacket)
{
    bool bRetVal = false;

    if (psPacket)
    {
        switch (psPacket->PacketType)
        {
            case CMD : ProcessCmd(psPacket->pucPayload);
                    break;
            case RESP: ProcessResp(psPacket->pucPayload);
                    break;
            case DATA: //ProcessData
                    break;
            case ACK:  //ProcessAcknowlodge
                    break;
            default  :
                    break;
        }

        bRetVal = true;
    }

    return bRetVal;
}

/**
 * @brief      : parseWifiCred
 * @param [in] : pcCmd
 * @param [out]: pcCredential
 * @return     : None
*/
void parseWifiCred(const char *pcCmd, char *pcCredential)
{
    char cSSID[20] = {0};
    char cPassword[50] = {0};
#ifdef NVS_ENABLE
    // int8_t uCredentialIdx = 0;
    // _sConfigData *psConfigData = NULL;

    // psConfigData = GetConfigData();
#endif

    if (sscanf(pcCmd, "ssid:%[^,],pwd:%s", cSSID, cPassword) == 2)
    {
        sprintf(pcCredential, "%s,%s", cSSID, cPassword);
        SetWifiCred(cSSID, cPassword);

#ifdef NVS_ENABLE
        // for (uCredentialIdx = 0;uCredentialIdx < 5; uCredentialIdx++) 
        // {

        //     if (psConfigData[uCredentialIdx].bCredAddStatus == true) 
        //     {
        //         continue;
        //     }
        //     else 
        //     {
        //         // memcpy(&psConfigData[uCredentialIdx].sWifiCred.ucSsid, cSSID, strlen(cSSID));
        //         // memcpy(&psConfigData[uCredentialIdx].sWifiCred.ucPwd, cPassword, strlen(cPassword));
        //         // psConfigData[uCredentialIdx].bWifiStatus = false;
        //         // psConfigData[uCredentialIdx].bCredAddStatus = true;
        //         break;
        //     }

        // }
#endif

    }
    else
    {
        printf("Error: Enter ssid and pwd in the format- ssid:<username>,pwd:<password>");  //no space after comma
        pcCredential[0] = '\0';
    }
    
}

/**
 * @brief      : Process command
 * @param [in] : 
 * @param [out]: None
 * @return     : true for success
*/
bool ProcessCmd(char *pcCmd)
{
    bool bRetVal = false;
    bool WifiStatus = false;
    char cBuffer[80] = {0};
    _sAtCmdHandle *psAtCmdHndler = NULL;
    
    psAtCmdHndler = GetATCmdHandle();
    WifiStatus    = GetWifiStatus();


     if (pcCmd)
    {
#ifdef nRF52840    
        if (strcmp(pcCmd, "CONNECT") == 0)
        {        
            SetDeviceState(DEVICE_CONNECTED);

        }
#endif
        if (strcmp(pcCmd, "DISCONNECT") == 0)
        {
            if(IsWiFiConnected())
            {
                SetDeviceState(WIFI_DEVICE);
            }
            else 
            {
                SetDeviceState(WAIT_CONNECTION);
            }
            
        }
        else if(strcmp(pcCmd, "LOCATION") == 0)
        {
            // if (IsLocationDataOK())    //wifi disconnected      
            // {
            //     printk("DEBUG: inside proccescmd");
            //     if(!WifiStatus)
            //     {
            //         printk("DEBUG: inside WifiStatus ");
            //         SendPayloadToBle();
            //     }
                
            // }
            // else
            // {
            //     printk("Didnt get location fix\n\r");
            // }

            if (!WifiStatus) 
            {
                SetDeviceState(BLE_CONNECTED);
            }
        }
        else if(strstr(pcCmd, "ssid") != NULL)
        {
            printk("Config: %s\n\r", pcCmd);
            parseWifiCred(pcCmd, cBuffer);
            // WriteCredToFlash();
            SetAPCredentials(cBuffer);
            DisconnectFromWiFi();
            printk("Config: %s\n\r", cBuffer);
            psAtCmdHndler[2].CmdHdlr(psAtCmdHndler[2].pcCmd, psAtCmdHndler[2].pcArgs, psAtCmdHndler[2].nArgsCount);    
            k_msleep(100);
            //bRetVal = ProcessWiFiMsgs_NVS();
            //SetDeviceState(DEV_IDLE);
        }
    }
}

static void UpdateStateAfterResponse(bool bStatus)
{
    _eDevState *pDevState = 0;
    bool WifiStatus = false;

    pDevState = GetDeviceState();
    WifiStatus    = GetWifiStatus();

    switch(*pDevState)
    {
        case WAIT_CONNECTION:
                            if (bStatus)
                            {
                                BleStatusFlag = true;
                                SetBleStatus(true);
                                printk("DEBUG : flag true???????????\n\r");
                                if (!WifiStatus) 
                                {
                                    SetDeviceState(BLE_CONNECTED);
                                }
                                //SetDeviceState(BLE_CONNECTED);
                            }
                            else
                            {
                                BleStatusFlag = false;                                 
                                SetBleStatus(false);
                                printk("DEBUG : flag false???????????\n\r");
                                if(IsWiFiConnected())
                                {
                                   SetDeviceState(WIFI_DEVICE); 
                                }
                                else
                                {
                                    SetDeviceState(WAIT_CONNECTION);
                                }
                               
                            }
                            break;

        default             :
                            break;
    }
}

/**
 * @brief      : Process response
 * @param [in] : None
 * @param [out]: None
 * @return     : true for success
*/
bool ProcessResp(char *pcResp)
{
    bool bRetVal = false;

    if (pcResp)
    {
        if (strcmp(pcResp, "ACK") == 0)
        {
            UpdateStateAfterResponse(true);
        }
        else if (strcmp(pcResp, "NACK") == 0)
        {
            UpdateStateAfterResponse(false);
        }

        bRetVal = true;
    }

    return bRetVal;
}

/**
 * @brief      : Process payload
 * @param [in] : pcPayload - payload
 * @param [out]: None
 * @return     : true for success
*/
bool ProcessPayload(char *pcPayload)
{
    bool bRetVal = false;

    if (pcPayload)
    {
        bRetVal = true;
    }

    return bRetVal;
}

int WriteCredToFlash()
{
    int nRetVal = 0;
    uint8_t uReadBuf[255] = {0};
    uint8_t uCredentialIdx = 0;

#ifdef NVS_ENABLE
    _sConfigData sConfigData[5] = {0};
    _sConfigData *psConfigData = NULL;

    psConfigData = GetConfigData();
    // memcpy(&sConfigData, psConfigData, sizeof(_sConfigData));
    memcpy(sConfigData, psConfigData, sizeof(_sConfigData) * 5);
    printk("DEBUG : Config Data SSID %s\n", sConfigData[1].sWifiCred.ucSsid);
    nRetVal = NvsWrite((uint8_t *)sConfigData, sizeof(_sConfigData) * 5, CONFIG_IDX);

    if (nRetVal == (sizeof(_sConfigData) * 5)) 
    {
        printk("DEBUG : Config Data Updated Successfully\n\r");

        nRetVal = NvsRead(uReadBuf, sizeof(_sConfigData) * 5 , CONFIG_IDX); 
        memcpy(psConfigData, (_sConfigData *)uReadBuf, sizeof(_sConfigData) * 5);
        for (uCredentialIdx = 0;uCredentialIdx < 5; uCredentialIdx++) 
        {
            if (psConfigData[uCredentialIdx].bCredAddStatus == true) 
            {
                printk("DEBUG : Wifi SSID : %s\n", psConfigData[uCredentialIdx].sWifiCred.ucSsid);
                printk("DEBUG : Wifi Pwd : %s\n", psConfigData[uCredentialIdx].sWifiCred.ucPwd);
                printk("DEBUG : Wifi Last Connected status : %d\n", psConfigData[uCredentialIdx].bWifiStatus);
            }
            else 
            {
                continue;
            }
        }       
    }
    else 
    {
        printk("DEBUG : Failed to write credentials to device with Return value: %d\n", nRetVal);
    }
#endif

    return nRetVal;
}

//EOF