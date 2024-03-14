/**
 * @file BleService.c
 * @brief File containing Visense service related handling
 * @author
 * @see BleService.h
 * @date 27-09-2023
*/
 
/**************************** INCLUDES******************************************/
#include "BleService.h"
#include <string.h>

// #include <stdbool.h>
// #include <zephyr/bluetooth/bluetooth.h>
// #include <zephyr/bluetooth/hci.h>
// #include <zephyr/bluetooth/conn.h>
// #include <zephyr/bluetooth/uuid.h>
// #include <zephyr/bluetooth/gatt.h>
 
/**************************** MACROS********************************************/
#define SND_MAX_LEN 247
/* Custom Service Variables */
// #define BT_UUID_CUSTOM_SERVICE_VAL \
//     BT_UUID_128_ENCODE(0xe076567e, 0x5d3b, 0x11ee, 0x8c99, 0x0242ac120002)
 
/**************************** GLOBALS*******************************************/
// static struct bt_uuid_128 sServiceUUID = BT_UUID_INIT_128(
//     BT_UUID_CUSTOM_SERVICE_VAL);
 
 static struct bt_uuid_128 LocationData = BT_UUID_INIT_128(
     BT_UUID_128_ENCODE(0x5844ec64, 0x8371, 0x11ee, 0xb962, 0x0242ac120002));
static struct bt_uuid_128 WiFiId = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x5adee837, 0x8269, 0x4653, 0xaaee, 0xdd02b91b9cd6));
static struct bt_uuid_128 WiFiPwd = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0xc325307e, 0xfd1e, 0x4a57, 0x8593, 0x65a8a5f34851));

static uint8_t userData[248] = {0};
static uint8_t wssid[15] = {0};
static uint8_t wpwd[15] = {0};
char cSSID[15] = {0};
char cPwd[15] = {0};
static bool bNotificationEnabled = false;
static bool bConnected = false;
static bool wNotificationEnabled = false;
struct bt_conn *psConnHandle = NULL;
bool pwdchange = false;

/****************************FUNCTION DEFINITIONS********************************/

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
    char arr[10]= "Heello";
    value=arr; 
 
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
   // uint32_t ucbuff = 0;
 
    if (offset + len > SND_MAX_LEN) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
 
    memcpy(value + offset, buf, len);
    
    return len;
}

/**
 * @brief WiFi ssid Write callback
 * @param bt_conn - Connection handle
 * @param attr - GATT attributes
 * @param buf
 * @param len
 * @param offset
 * @return Length of the data read
*/
static ssize_t SSidWrite(struct bt_conn *conn, const struct bt_gatt_attr *attr,
             const void *buf, uint16_t len, uint16_t offset,
             uint8_t flags)
{
    uint8_t *value = attr->user_data;

   // uint32_t ucbuff = 0;
 
    if (offset + len > SND_MAX_LEN) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
 
    memcpy(value + offset, buf, len);
    memset(cSSID, 0, sizeof(cSSID));
    memcpy(cSSID, value, len);

    printk("\n\r SSID: %s\r\n", cSSID);  //check the recieved ssid from app user

    printk("\n\r SSID len is : %d\r\n", len);
    return len;
}

/**
 * @brief WiFi password Write callback
 * @param bt_conn - Connection handle
 * @param attr - GATT attributes
 * @param buf
 * @param len
 * @param offset
 * @return Length of the data read
*/
static ssize_t PwdWrite(struct bt_conn *conn, const struct bt_gatt_attr *attr,
             const void *buf, uint16_t len, uint16_t offset,
             uint8_t flags)
{
    uint8_t *value = attr->user_data;
   // uint32_t ucbuff = 0;
 
    if (offset + len > SND_MAX_LEN) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    memcpy(value + offset, buf, len);
    memset(cPwd, 0, sizeof(cPwd));
    memcpy(cPwd, value, len);

    printk("\n\r password: %s\r\n", cPwd);  //check if rcvd correctly frm app
    pwdchange = true; 
    // printk("\n\r set pwdchage true... : %d", pwdchange);
    return len;
}

/* to check if wifi password is changed from the ble app*/
bool ifpwdchange()
{
    return pwdchange;
}

/* to change the passwordchange condition back to false*/
bool amendpwd()
{
    pwdchange = false;
    // printk("\n\r set pwdchage : %d", pwdchange);
    return false;
}

/* to get the Wifi credential values set from the nrf app*/
void getWiFiCred( char** uname, char** pword)
{
    *pword = cPwd; 
    *uname = cSSID; 
    printk("\n\rwifi cred r %s and %s\r\n", *uname, *pword); 
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

void WiFi_pwd_Notify(const struct bt_gatt_attr *attr, uint16_t value)
{
    if (value == BT_GATT_CCC_NOTIFY)
    {
        wNotificationEnabled = true;
    }
    else
    {
        wNotificationEnabled = false;
    }
}


 
/* LOCATION SERVICE DEFINITION*/
/**
 * @note Service registration and chara adding.
 * @paragraph Below service has one chara with a notify permission.
 * */
 BT_GATT_SERVICE_DEFINE(LocationService,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_LNS),
    BT_GATT_CHARACTERISTIC(&LocationData.uuid,
                BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_READ,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                CharaRead, CharaWrite, userData),
    BT_GATT_CCC(BleSensorDataNotify, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(&WiFiId.uuid,                      //to accept new wifi credentials from the app
                BT_GATT_CHRC_WRITE,
                BT_GATT_PERM_WRITE,
                NULL, SSidWrite, wssid),
    BT_GATT_CHARACTERISTIC(&WiFiPwd.uuid,                       //pwd change from the app
                BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
                BT_GATT_PERM_WRITE,
                NULL, PwdWrite, wpwd),
    BT_GATT_CCC(WiFi_pwd_Notify,BT_GATT_PERM_WRITE)
);

// );

 
 
/**
 * @brief Connect callabcak
*/
static void connected(struct bt_conn *conn, uint8_t err)
{
 
    if (err)
    {
        printk("Connection failed (err 0x%02x)\n", err);
    }
    else
    {
        bt_conn_le_data_len_update(conn, BT_LE_DATA_LEN_PARAM_MAX);
        bConnected = true;
    }
}
 
/**
 * @brief Disconnect callback
*/
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    bConnected = false;
    printk("Disconnected (reason 0x%02x)\n", reason);
}
 
/**
 * @note Callback definitions for connection
*/
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
int LocationSensordataNotify(void * pucSensorData, uint16_t unLen)
{
    int nRetVal = 0;
 
    if (pucSensorData)
    {
        nRetVal = bt_gatt_notify(NULL, &LocationService.attrs[1],
                                pucSensorData, unLen);
    }
 
    return nRetVal;
}

/**
 * @brief Sending Wifi credentials verification status as notification
 * @param Wificonnect - Status to notify
 * @return 0 in case of success or negative value in case of error
*/
int WiFiConnectNotify(void * Wificonnect, uint16_t unLen)
{
    int nRetVal = 0;
    //printk("attribute count is %d\n", LocationService.attr_count);
    if (Wificonnect)
    {
        nRetVal = bt_gatt_notify(NULL, &LocationService.attrs[7],
                                Wificonnect, unLen);
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
 * @brief Check if wifi connection status is ok
 * @param None
 * @return returns if notifications is enabled or not.
*/
bool WiFiConnectOK()
{
    return wNotificationEnabled;
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
