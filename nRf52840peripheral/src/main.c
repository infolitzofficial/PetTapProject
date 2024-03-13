/**
 * @file main.c
 * @brief Main function
 * @date 2023-21-09
 * @author Jeslin
 * @note  This is a test code for pressure sensor.
*/


/*******************************INCLUDES****************************************/
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <nrfx_example.h>
#include <saadc_examples_common.h>
#include <nrfx_saadc.h>
#include <nrfx_log.h>
#include <stdlib.h>
#include <stdio.h>
#include "BleHandler.h"
#include "BleService.h"
#include "UartHandler.h"
#include "PacketHandler/PacketHandler.h"
#include "System/SystemHandler.h"


/*******************************MACROS****************************************/


/*******************************GLOBAL VARIABLES********************************/
uint8_t ucNotifyBuf[] = {0xdd, 0x03, 0x00, 0x1b, 0x05, 0x34, 0x00, 0x00, 0x19, 0xab, 0x27, 0x10, 0x00, 0x00, 0x2a, 0x75, 0x00};

uint8_t ucNotifyBuf2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x42, 0x03, 0x04, 0x02, 0x09, 0x7f, 0x0b,0xa3, 0xfc, 0x71, 0x77};

/*******************************FUNCTION DEFINITIONS********************************/


/**
 * @brief  Main function
 * @return int
*/
int main(void)
{
    int nError;
    // uint8_t ucRcvdbuf[100] = {0};
    // uint8_t ucSampleBuf[] = {0xDD,0xA5,0x03,0x00,0xFF,0xFD,0x77};


    if (!EnableBLE())
    {
        printk("Bluetooth init failed (err %d)\n", nError);
        return 0;
    }
    if(!InitUart())
    {
        printk("Failed to init uart module\n\r");
        return 0;
    }

    StartAdvertising();

    while(1)
    {
        PollMsgs();
        ProcessDeviceState();
    }

    printk("Program exit");

    return;
}