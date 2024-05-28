
#ifndef _NVS_HANDLER_H
#define _NVS_HANDLER_H

/***************************************INCLUDES*******************************************/
#include <stdio.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include "../WiFi/WiFiHandler.h"

/***************************************MACROS********************************************/
/*Uncomment the below macro for enable Flash storage functionality*/
#define NVS_ENABLE 
/*****************************************************************************************/
#ifdef  NVS_ENABLE

#define NVS_PARTITION		storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)

#define CONFIG_IDX              0
#define ENOREDY              0xFF



/****************************************TypeDefs******************************************/

typedef struct __attribute__((packed)) __sConfigTimes
{
    uint16_t usWifiTimeout;
    uint16_t usBleTimeout;
    uint16_t usLteTimeout;
}_sConfigTimes;

typedef struct __attribute__((packed)) __sConfigData
{
    _sWifiCred sWifiCred;
    _sConfigTimes sConfigTimes;
    bool bWifiStatus;
    bool bCredAddStatus;
}_sConfigData;



/***************************************FUNCTION DECLARATION********************************/

int NvsInit();
int NvsRead(uint8_t *puBuf, uint16_t usBufLen, uint16_t usIdx); 
int NvsWrite(uint8_t *puBuf, uint16_t usBufLen, uint16_t usIdx);
int NvsDelete(uint16_t usIdx);
_sConfigData *GetConfigData();

#endif
#endif