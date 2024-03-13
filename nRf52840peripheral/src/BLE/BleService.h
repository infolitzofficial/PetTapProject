/**
 * @file BleService.h
 * @brief File containing Visense service related handling
 * @author
 * @see BleService.c
 * @date 27-09-2023
*/

/**************************************INCLUDES******************************/
#include "BleHandler.h"

/***************************************MACROS*******************************/
/* Custom Service Variables */
#define BT_UUID_CUSTOM_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x0000ff00,0x0000,0x1000,0x8000,0x00805f9b34fb)
/**************************************TYPEDEFS******************************/

/*************************************FUNCTION DECLARATION*******************/
int LocationdataNotify(uint8_t *pucSensorData, uint16_t unLen);
void BleSensorDataNotify(const struct bt_gatt_attr *attr, uint16_t value);
bool IsNotificationenabled();
bool IsConnected();
void GetRcvdData(uint8_t *pucData);
bool IsDataRcvd();
void SetRcvdDataStatus(bool bStatus);