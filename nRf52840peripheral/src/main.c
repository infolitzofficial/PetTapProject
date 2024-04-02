/**
 * @file     : main.c
 * @brief    : Application for PetTap BLE services
 * @date     : 15-03-2024
 * @author   : Adhil
 * @note     : None 
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
#include "Nfc.h"


/*******************************MACROS****************************************/

/*******************************GLOBAL VARIABLES********************************/

/*******************************FUNCTION DEFINITIONS********************************/
/**
 * @brief      : Main function
 * @param [in] : None
 * @param [out]: None
 * @return     :
*/
int main(void)
{
    int nError;

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

    nfc_Setup();


    while(1)
    {
        PollMsgs();
        ProcessDeviceState();
    }

    printk("CRITICAL: Program exit");

    return;
}

//EOF