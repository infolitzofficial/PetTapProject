/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/_stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/printk.h>
#include "zephyr/sys/util.h"
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>

#include <dk_buttons_and_leds.h>
#include "BleService.h"

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED          DK_LED1
#define CON_STATUS_LED          DK_LED2
#define RUN_LED_BLINK_INTERVAL  1000
#define NOTIFY_INTERVAL         1000

/*UART for AT commands via DA16200*/
/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
#define WIFI_CONN_DELAY 30000
#define WIFI_SSID_PWD	"Alcodex,Adx@2013"
#define WIFI_SSID_PWD1	"Maker_GUEST,WiFi4M4KEr"
#define AWS_BROKER		"a1kzdt4nun8bnh-ats.iot.ap-northeast-2.amazonaws.com"
#define AWS_THING 		"test_aws_iot"
#define AWS_TOPIC 		"test_aws_iot/testtopic"
#define CFG_NUM 	1
#define CFG_NAME 	"latlong"
#define MSG_SIZE 	32
#define BUF_SIZE 	98

  
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);
 
/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
char temp_buf[BUF_SIZE];
static int rx_buf_pos;
static bool bRcvdData = false;
char cLat[] = "10.059065067392345W,76.34034918061742N";
char latda[] = "20.059065067392345N76.54034918061742E";
char cStat[20]={0};
char wCred[50]= {0};

static void start_advertising(void);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)), 
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

bool checkConnection()
{
	print_uart("AT+WFSTA\r\n");
	k_msleep(500);
	if (strcmp(cStat, "WiFi connected!") == 0)
	{
		// printk("Connected to WiFi\n");
		return true;
	}
	else if (strcmp(cStat, "No connection!") == 0)
	{
		// printk("No WiFI available\n");
		return false;
	}
	//return false;
}

bool checkifIdRegistered(char *uname1)
{
	print_uart("AT+WFJAPA=?\r\n");
	k_msleep(300);
	// printk("Registered buffer- %s, Uname- %s\n", wCred, uname1);
	if (strstr(wCred, uname1) != NULL)
	{
		return true;
	}
	else
	{
		return false;
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;
	struct bt_conn_info info;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Connection failed (err %d)\n", conn_err);
		return;
	}

	err = bt_conn_get_info(conn, &info);
	if (err) {
		printk("Failed to get connection info (err %d)\n", err);
	} else {
		const struct bt_conn_le_phy_info *phy_info;
		phy_info = info.le.phy;

		// printk("Connected: %s, tx_phy %u, rx_phy %u\n",
		//        addr, phy_info->tx_phy, phy_info->rx_phy);
	}
	dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
	dk_set_led_off(CON_STATUS_LED);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};


static void start_advertising(void)
{
	int err;
	err =  bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Failed to start advertising set (err %d)\n", err);
		return;
	}

}
 
/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
    uint8_t c;
 
    if (!uart_irq_update(uart_dev)) {
        return;
    }
 
    if (!uart_irq_rx_ready(uart_dev)) {
        return;
    }
    /* read until FIFO empty */
    while (uart_fifo_read(uart_dev, &c, 1) == 1)
    {
        if ((c == '\n' || c == '\r') && rx_buf_pos > 0)
        {
            /* terminate string */
            rx_buf[rx_buf_pos] = '\0';

			//To check Wifi connection status
			if (strstr(rx_buf, "WFSTA:1") != NULL)    //AT command response will be WFSTA:1 when connection success
			{
				strcpy(cStat,"WiFi connected!");
				printk("WiFI ok\n");
			}
			else if (strstr(rx_buf, "WFSTA:0") != NULL)
			{
				strcpy(cStat,"No connection!");
				printk("No WiFI\n");
			}


			//To check if the user given wiFi credentials are processed by the DA
			if(strstr(rx_buf, "WFJAPA:") != NULL)
			{
				const char *startPtr = rx_buf + 2;
				//strncpy(wCred, startPtr, length);
				strcpy(wCred, startPtr);
				printk("cred copied to wCred:- %s\n", wCred);
			}

			bRcvdData = true;          
            /* reset the buffer (use over) */
            memset(rx_buf,0,strlen(rx_buf));
            rx_buf_pos = 0;        
        }
        else if (rx_buf_pos < (sizeof(rx_buf) - 1))
        {
            rx_buf[rx_buf_pos++] = c;
        }      
    }  
}
 
/*
 * Print a null-terminated string character by character to the UART interface
 */
void print_uart(char *buf)
{
    int msg_len = strlen(buf);
 
    for (int i = 0; i < msg_len; i++) {
        uart_poll_out(uart_dev, buf[i]);
    }
}


/**
* Sends the sequence of the required AT commands via uart interface to DA16200 module
*/
#define MAX_CMDS 7
void sendMyCommand( int8_t num )
{
	switch(num)
	{
		case 1:
			print_uart("AT+WFMODE=0\r\n");  //enable station mode
			break;
		case 2:
			sprintf(temp_buf, "AT+WFJAPA=%s\r\n", WIFI_SSID_PWD); //connect to wifi
			print_uart(temp_buf);
			k_msleep(WIFI_CONN_DELAY);	//min advisable delay 25000
			if (!checkConnection())
			{
				printk("Tried %s - Not Connecting... Trying %s...\n", WIFI_SSID_PWD, WIFI_SSID_PWD1);
				sprintf(temp_buf, "AT+WFJAPA=%s\r\n", WIFI_SSID_PWD1); //connecting to back up wifi due to previous connect failure
				print_uart(temp_buf);
				k_msleep(WIFI_CONN_DELAY);
				if (!checkConnection())
				{
					printk("No WiFI available\n");		//Primary and Back up Wifi Not working
				}
				else
				{
					printk("Connected to %s\n", WIFI_SSID_PWD1);
				}
			}
			else if (checkConnection())		//double check connection status
			{
				printk("Connected to %s\n", WIFI_SSID_PWD);
			}
			break;
		case 3:
			sprintf(temp_buf, "AT+AWS=SET APP_PUBTOPIC %s\r\n", AWS_TOPIC);  //lport 8883 or 18883 or AWS port 443
			print_uart(temp_buf);
			k_msleep(1000);
			break;
		case 4:
			print_uart("AT+AWS=CFG  0 latshad 1 1\r\n"); 		//shadow config
			k_msleep(10000);
			break;
		case 5:
			print_uart("AT+AWS=CMD MCU_DATA 0 latshad init\r\n");   //shadow sensor update
			k_msleep(20000);    //20secs wait time
			break;
		case 6:
			sprintf(temp_buf, "AT+AWS=CFG %d %s 1 0\r\n", CFG_NUM, CFG_NAME);
			print_uart(temp_buf);
			k_msleep(1000);
			break;
		case 7:
			sprintf(temp_buf, "AT+AWS=CMD MCU_DATA %d %s %s\r\n", CFG_NUM, CFG_NAME, latda);    //data sends
			print_uart(temp_buf);
			k_msleep(20000);    //20secs delay between sends
			break;
		default:
			break;
	}
}

int main(void)
{
	uint32_t led_status = 0;
	int err;
	int count = 0;
	char *uname;
	char *pword;
	int8_t cmdID = 1;
	char temp_buf[BUF_SIZE];
	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return 0;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");
	
	start_advertising();

	//UART config

    if (!device_is_ready(uart_dev))
    {
        printk("UART device not found!");
        return 0;
    }

    /* configure interrupt and callback to receive data */
    int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
 
    if (ret < 0)
    {
        if (ret == -ENOTSUP) {
            printk("Interrupt-driven UART API support not enabled\n");
        } else if (ret == -ENOSYS) {
            printk("UART device does not support interrupt-driven API\n");
        } else {
            printk("Error setting UART callback: %d\n", ret);
        }
        return 0;
    }
	
    uart_irq_rx_enable(uart_dev);
    k_msleep(1000);
    print_uart("ATZ\r\n");
    k_msleep(1000);

	for (;;) 
	{
		dk_set_led(RUN_STATUS_LED, (++led_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));

		if(IsNotificationenabled())
		{
			LocationSensordataNotify(cLat, sizeof(cLat));		//send location data recieved notify to the app user
		}
		if(WiFiConnectOK())  
		{
			WiFiConnectNotify(cStat, sizeof(cStat));		//notify the app user of the wiFI connection status if password modified
		}

		if (ifpwdchange())		//set to execute only if the user changes the WiFi password from the BLE nrF App
		{
			int8_t chk = 0;
rep:		getWiFiCred(&uname,&pword);   //use rep: here
			sprintf(temp_buf, "AT+WFJAPA=%s,%s\r\n", uname, pword);
        	print_uart(temp_buf);
			k_msleep(20000);
			if(!checkifIdRegistered(uname))
			{
				chk++;
				if (chk < 10)
				{
					printk("Not registered...try again\n");
					goto rep;
				}
				else
				{
					printk("Not registering the new ssid %s.. Quit trying\n",uname );
					chk = 0;
				}
			}
			amendpwd();			//to make the pwdchange condition false
			k_msleep(10000);
			if(checkConnection())
			{
				cmdID = 3;	//if wifi cred changed by the user and connect is success, skip case 2 or strat from shadow config
			}
			else
			{
				printk("N/w - %s - not connected! \n", uname);
			}
		}
		else if(bRcvdData)	//proceed with AT commands only after UART response is received
        {
		    bRcvdData = false;
            sendMyCommand(cmdID);
			if(cmdID != 7)
            {
				cmdID++;
            }

			count ++;
			if (count > 10)  //optional- stop publishing and reset to try wifi connect if no n/w available
			{
				if (!checkConnection())
				{
					cmdID = 2;
					printk("chk-chk connectn lost.  Try Autoconnect! \n");
				}
				count = 0;
			}
        }
	}
}
