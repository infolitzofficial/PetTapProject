/**
 * @file 	: BleService.c
 * @brief 	: File containing PetTap service related handling
 * @author	: Adhil
 * @see 	: BleService.h
 * @date	: 15-03-2024
*/

/**************************** INCLUDES******************************************/
#include "BleService.h"
#include "UartHandler.h"
#include "../System/SystemHandler.h"

/**************************** MACROS********************************************/
#define VND_MAX_LEN 255


/**************************** GLOBALS*******************************************/
static struct bt_uuid_128 sServiceUUID = BT_UUID_INIT_128(
	BT_UUID_CUSTOM_SERVICE_VAL);

static struct bt_uuid_128 sUartReadChara = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x0000ff01,0x0000,0x1000,0x8000,0x00805f9b34fb));

static struct bt_uuid_128 sUartResponseChara = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x0000ff02,0x0000,0x1000,0x8000,0x00805f9b34fb));

static uint8_t ucSensorData[VND_MAX_LEN + 1] = {0x11,0x22,0x33, 0x44, 0x55};
static uint8_t ucWriteBuf[100] = {0};
static bool bNotificationEnabled = false; 
static bool bConnected = false;
struct bt_conn *psConnHandle = NULL;
static bool bRcvdData = false;

/****************************FUNCTION DEFINITION********************************/

/**
 * @brief 	   : Charcteristics Read callback
 * @param [in] : bt_conn - Connection handle
 * @param [in] : attr - GATT attributes
 * @param [in] : buf 
 * @param [in] : len 
 * @param [in] : offset
 * @return 	   : Length of the data read
*/
static ssize_t CharaRead(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

/**
 * @brief 	   : Charcteristics Write callback
 * @param [in] : bt_conn - Connection handle
 * @param [in] : attr - GATT attributes
 * @param [in] : buf 
 * @param [in] : len 
 * @param [in] : offset
 * @return 	   : Length of the data read
*/
static ssize_t CharaWrite(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset + len > VND_MAX_LEN) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	memcpy(ucWriteBuf, value, len);
	SetDeviceState(BLE_CONFIG);
	bRcvdData = true;

	return len;
}

/**
 * @brief MTU exchange callback
 * @param conn 	  : Connection handle
 * @param att_err : Error code on MTU exchange
 * @param params  : Exchange params
 * @return None
*/
static void MTUExchangeCb(struct bt_conn *conn, uint8_t att_err,
    					struct bt_gatt_exchange_params *params)
{
    if (att_err)
    {
        printk("\n\rMTU exchange returned with error code %d\n\r", att_err);
    }
    else
    {
        printk("\n\rMTU sucessfully set to %d\n\r", CONFIG_BT_L2CAP_TX_MTU);
    }
}

/**
 * @brief Initiate MTU exchange from peripheral side
 * @param conn  : connection handle
 * @return None
*/
static void InitiateMTUExcahnge(struct bt_conn *conn)
{
    int err;
    static struct bt_gatt_exchange_params exchange_params;
    exchange_params.func = MTUExchangeCb;

    err = bt_gatt_exchange_mtu(conn, &exchange_params);
    if (err)
    {
        printk("\n\rMTU exchange failed (err %d)\n\r", err);
    }
    else
    {
        printk("\n\rMTU exchange pending ...\n\r");
    }
}

/**
 * @brief 	   : Notification callback
 * @param [in] : attr - pointer to GATT attributes
 * @param [in] : value - Client Characteristic Configuration Values
 * @return 	   : None
*/
void BleSensorDataNotify(const struct bt_gatt_attr *attr, uint16_t value)
{

    if (value == BT_GATT_CCC_NOTIFY)
    {
        bNotificationEnabled = true;
    }
    else
    {
        bNotificationEnabled = false;
    }
}

/* PETTAP SERVICE DEFINITION*/
/**
 * @note Service registration and chara adding.
 * @paragraph Below service has one chara with a notify permission.
*/
BT_GATT_SERVICE_DEFINE(PetTapService,
    BT_GATT_PRIMARY_SERVICE(&sServiceUUID),
    BT_GATT_CHARACTERISTIC(&sUartReadChara.uuid,
                BT_GATT_CHRC_NOTIFY,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                CharaRead, CharaWrite, ucSensorData),
    BT_GATT_CCC(BleSensorDataNotify, (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)),
	BT_GATT_CHARACTERISTIC(&sUartResponseChara.uuid,
				BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
					BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
					CharaRead,CharaWrite,ucSensorData)
);

/**
 * @brief 	   : Connection callback
 * @param [in] : err - Error code
 * @param [in] : conn - Connection handle
 * @return	   : None
*/
static void connected(struct bt_conn *conn, uint8_t err)
{
	bConnected = true;
	printk("Connected\n");
	InitiateMTUExcahnge(conn);
}

/**
 * @brief 	   : Disconnection callback
 * @param [in] : err - Error code
 * @param [in] : conn - Connection handle
 * @return	   : None
*/
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	bConnected = false;
	SetDeviceState(BLE_DISCONNECTED);
	printk("Disconnected (reason 0x%02x)\n", reason);
}

/**
 * Ble Event Callback functions are registered here
*/
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/**
 * @brief 	   : Sending location data as notification
 * @param [in] : pucSensorData - Data to notify
 * @param [in] : unLen - length of data to notify
 * @return 	   : 0 in case of success or negative value in case of error
*/
int LocationdataNotify(uint8_t *pucSensorData, uint16_t unLen)
{
	int nRetVal = 0;

    if (pucSensorData)
    {
	    nRetVal = bt_gatt_notify(NULL, &PetTapService.attrs[1], 
                                pucSensorData, unLen);
    }

	return nRetVal;
}

/**
 * @brief 	  : Check if notification is enabled
 * @param     : None
 * @return 	  : returns if notifications is enabled or not.
*/
bool IsNotificationenabled()
{
    return bNotificationEnabled;
}

/**
 * @brief 	  : Check if the device connected
 * @param     : None
 * @return    : returns the device is connected or not
*/
bool IsConnected()
{
	return bConnected;
}


/**
 * @brief 	   : Get received data
 * @param [in] : None
 * @param [out]: None 
 * @return     : pointer to received data
*/
void GetRcvdData(uint8_t *pucData)
{
	if (pucData)
	{
		memcpy(pucData, ucWriteBuf, sizeof(ucWriteBuf));
	}
}

/**
 * @brief      : Check if data is received
 * @param [in] : None
 * @param [out]: None
 * @return true for success
*/
bool IsDataRcvd()
{
	return bRcvdData;
}


/**
 * @brief 	   : Set received data status
 * @param [in] : bStatus
 * @param [out]: None
 * @return 	   : None
*/
void SetRcvdDataStatus(bool bStatus)
{
	bRcvdData = bStatus;
}

//EOF