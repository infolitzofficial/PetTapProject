/**
 * @file BleService.h
 * @brief File containing Visense service related handling
 * @author
 * @see BleService.c
 * @date 27-09-2023
*/
 
/**************************************INCLUDES******************************/
#include <stdbool.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

/***************************************MACROS*******************************/
 
/**************************************TYPEDEFS******************************/
 
/*************************************FUNCTION DECLARATION*******************/
int LocationSensordataNotify(void *pucSensorData, uint16_t unLen);
int WiFiConnectNotify(void *Wificonnect, uint16_t unLen);
void BleSensorDataNotify(const struct bt_gatt_attr *attr, uint16_t value);
bool IsNotificationenabled();
bool WiFiConnectOK();
bool IsConnected();
void getWiFiCred( char** ssid, char** pswd);
bool ifpwdchange();
bool amendpwd();