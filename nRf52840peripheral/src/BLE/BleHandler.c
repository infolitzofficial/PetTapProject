/**
 * @file   : Blehandler.c
 * @author : Adhil
 * @brief  : Files containing Ble functions
 * @date   : 15-03-2024
 * 
*/

/***********************************INCLUDES**************************/
#include "BleHandler.h"
#include "BleService.h"

/************************************MACROS***************************/
#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

/************************************GLOBALS**************************/
struct bt_le_ext_adv *adv; //Advertsisement handle
uint8_t ucAdVertsingBuffer[ADV_BUFF_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00}; //Advertsising buffer

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_SHORTENED, DEVICE_NAME, DEVICE_NAME_LEN)
};

/**********************************FUNCTION DEFINITION****************/
/**
 * @brief      : This function is for Enabling BLE
 * @param [in] : None 
 * @param [out]: None
 * @return     : True for success
*/
bool EnableBLE()
{
    int nError = 0;
    bool bRetVal = false;

    nError = bt_enable(NULL);

	if (!nError) 
    {
        bRetVal = true;
	}
    else
    {
		printk("Bluetooth init failed (err %d)\n", nError);
    }

    return bRetVal;
}

/**
 * @brief      : function to initialize extended advertising
 * @param [in] : None 
 * @param [out]: None
 * @return     : nRetVal - 0 for success
*/
int InitExtendedAdv(void)
{
	int nRetVal = 0;
	struct bt_le_adv_param param =
		BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_CONNECTABLE |
				     BT_LE_ADV_OPT_EXT_ADV,
				     BT_GAP_ADV_FAST_INT_MIN_2,
				     BT_GAP_ADV_FAST_INT_MAX_2,
				     NULL);

	nRetVal = bt_le_ext_adv_create(&param, NULL, &adv);
	do
    {
        if (nRetVal) 
        {
		    printk("Failed to create advertiser set (err %d)\n", nRetVal);
		    break;
	    }

	    printk("Created adv: %p\n", adv);
	    nRetVal = bt_le_ext_adv_set_data(adv, ad,ARRAY_SIZE(ad), NULL, 0);

	    if (nRetVal) 
        {
	    	printk("Failed to set advertising data (err %d)\n", nRetVal);
	    	break;
	    }

    } while(0);
    
	return nRetVal;
}


/**
 * @brief      : function to start advertising
 * @param [in] : None 
 * @param [out]: None
 * @return     : nError - 0 for success
*/
int StartAdvertising(void)
{
	int nError = 0;

	nError = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);

	if (nError) 
    {
		printk("Failed to start advertising set (err %d)\n", nError);
	}
    else
    {
        printk("Advertiser set started\n");
    }

    return nError;
}


/**
 * @brief      : Getting advertising buffer 
 * @param [in] : None 
 * @param [out]: None
 * @return     : address of advertising buffer
*/
uint8_t *GetAdvertisingBuffer()
{
    return ucAdVertsingBuffer;
}

/**
 * @brief      : Stop advertsisement
 * @param [in] : None
 * @param [out]: None
 * @return     : true for success
*/
bool BleStopAdvertise()
{
    int nError = 0;
    bool bRetVal = false;

    nError = bt_le_ext_adv_stop(adv);

 	if (!nError) 
    {
        bRetVal = true;
	}
    else
    {
		printk("Bluetooth init failed (err %d)\n", nError);
    }

    return bRetVal;
}

//EOF