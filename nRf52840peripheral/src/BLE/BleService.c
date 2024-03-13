/**
 * @file BleService.c
 * @brief File containing Visense service related handling
 * @author
 * @see BleService.h
 * @date 27-09-2023
*/

/**************************** INCLUDES******************************************/
#include "BleService.h"
#include "UartHandler.h"

/**************************** MACROS********************************************/
#define VND_MAX_LEN 12


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
 * @brief Charcteristics Read callback
 * @param bt_conn - Connection handle
 * @param attr - GATT attributes
 * @param buf 
 * @param len 
 * @param offset
 * @return Length of the data read
*/
static ssize_t CharaRead(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

/**
 * @brief Charcteristics Write callback
 * @param bt_conn - Connection handle
 * @param attr - GATT attributes
 * @param buf 
 * @param len 
 * @param offset
 * @return Length of the data read
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
	// value[offset + len] = 0;
	memcpy(ucWriteBuf, value, len);
	bRcvdData = true;

	

	return len;
}

/**
 * @brief Notification callback
 * @param attr - pointer to GATT attributes
 * @param value - Client Characteristic Configuration Values
 * @return None
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

/* VSENCE SERVICE DEFINITION*/
/**
 * @note Service registration and chara adding.
 * @paragraph Below service has one chara with a notify permission.
*/
BT_GATT_SERVICE_DEFINE(VisenseService,
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

static void connected(struct bt_conn *conn, uint8_t err)
{
	printk("Connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	bConnected = false;
	printk("Disconnected (reason 0x%02x)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/**
 * @brief Sending sensor data as notification
 * @param pucSensorData - Data to notify
 * @param unLen - length of data to notify
 * @return 0 in case of success or negative value in case of error
*/
int VisenseSensordataNotify(uint8_t *pucSensorData, uint16_t unLen)
{
	int nRetVal = 0;

    if (pucSensorData)
    {
	    nRetVal = bt_gatt_notify(NULL, &VisenseService.attrs[1], 
                                pucSensorData, unLen);
    }

	return nRetVal;
}

/**
 * @brief Check if notification is enabled
 * @param None
 * @return returns if notifications is enabled or not.
*/
bool IsNotificationenabled()
{
    return bNotificationEnabled;
}

/**
 * @brief Check if the device connected
 * @param None
 * @return returns the device is connected or not
*/
bool IsConnected()
{
	return bConnected;
}


/**
 * @brief Get received data
 * @param 
 * @return pointer to received data
*/
void GetRcvdData(uint8_t *pucData)
{
	if (pucData)
	{
		memcpy(pucData, ucWriteBuf, sizeof(ucWriteBuf));
	}
}

/**
 * @brief Check if data is received
 * @param None
*/
bool IsDataRcvd()
{
	return bRcvdData;
}


/**
 * @brief Set received data status
 * @param bStatus
*/
void SetRcvdDataStatus(bool bStatus)
{
	bRcvdData = bStatus;
}
