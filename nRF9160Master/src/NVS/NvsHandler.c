

#include "NvsHandler.h"
#include "zephyr/fs/nvs.h"
#include "zephyr/sys/printk.h"
#include <ssp/string.h>
#include <stdio.h>

#ifdef  NVS_ENABLE


/*************************************************PRIVATE VARIABLE**********************************************/
static struct nvs_fs sNvsFileSystem = {0};

static _sConfigData 
sConfigData[5] = {
    {{"realme GT 5G", "s3qqyipp"}, true, true},
    {{0, 0}, false, false},
    {{0, 0}, false, false},
    {{0, 0}, false, false},
    {{0, 0}, false, false},
};


/*************************************************FUNCTION DEFINICTION*******************************************/

int NvsInit()
{
    int nRetVal = 0;
    struct flash_pages_info sPageInfo = {0};

    sNvsFileSystem.flash_device = NVS_PARTITION_DEVICE;

    if (!device_is_ready(sNvsFileSystem.flash_device)) 
    {
		printk("Flash device %s is not ready\n", sNvsFileSystem.flash_device->name);
		return ENOREDY;
	}

    sNvsFileSystem.offset = NVS_PARTITION_OFFSET;

    nRetVal = flash_get_page_info_by_offs(sNvsFileSystem.flash_device, sNvsFileSystem.offset, &sPageInfo);
    if (nRetVal) 
    {
        printk("Unable to get page info\n");
        return nRetVal;
    }

    sNvsFileSystem.sector_size = sPageInfo.size;
    sNvsFileSystem.sector_count = 3u;


    nRetVal = nvs_mount(&sNvsFileSystem);
    if(nRetVal)
    {
        printk("Flash Init failed\n");
		return nRetVal;
    }
    return nRetVal;

}
/**
*
*/
int NvsRead(uint8_t *puBuf, uint16_t usBufLen, uint16_t usIdx)
{
    int nRetVal = 0;

    memset(puBuf, 0, usBufLen);
    nRetVal = nvs_read(&sNvsFileSystem, usIdx, puBuf, usBufLen);
    if (nRetVal < 0) 
    {
        printk("ERROR: nvs_read failed");
        return nRetVal;
    }
    else
    {
        return nRetVal;
    }
}

int NvsWrite(uint8_t *puBuf, uint16_t usBufLen, uint16_t usIdx)
{
    int nretVal = 0;

    nretVal = nvs_write(&sNvsFileSystem,CONFIG_IDX,puBuf,usBufLen);

    printk("DEBUG: Write Buffer Length %d\n",nretVal);
    
    if (nretVal < 0)
    {
        printk("ERROR:nvs_write failed");
    
    }

    return nretVal;
}

int NvsDelete (uint16_t usIdx)
{
    int nretVal = 0;

    nretVal = nvs_delete(&sNvsFileSystem,usIdx);

    if (nretVal < 0)
    {
        printk("ERROR:nvs_delete failed");
        return nretVal;
    }
}

_sConfigData *GetConfigData()
{
    return sConfigData;
}

#endif