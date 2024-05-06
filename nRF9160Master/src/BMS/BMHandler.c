/**
 * @file    I2Cchargerhandler.c
 * @author : Devendu
 * @brief   File to get battery percentage
 * @date    25-04-2023
 */
/***********************************INCLUDES**************************/
#include "zephyr/sys/printk.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/_stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include "BMHandler.h"


/************************************MACROS***************************/


#define CHARGER2_AUTO_DETECT              0x00
#define CHARGER2_14_BITS_RESOLUTION       0x00
#define CHARGER2_OPERATING_MODE           0x10
#define SLAVE_ADDRESS                     0x70
#define RegVoltageLowByte                 0x08
#define RegVoltageHighByte                0x09
#define RegTemperatureLowByte             0x10
#define RegTemperatureHighByte            0x11


/************************************GLOBALS**************************/


/**********************************FUNCTION DEFINITIONS****************/


static const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c3));


/**
 * @brief Initializes the I2C charger device.
 *
 * This function initializes the I2C charger device by writing specific configuration data
 * to the appropriate register of the device. It checks for successful initialization
 * and prints debug messages accordingly.
 */


void InitI2CCharger(void)
{
        printf("wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\r\n");
        if(i2c_reg_write_byte(i2c_dev, SLAVE_ADDRESS, 0x00, CHARGER2_AUTO_DETECT | CHARGER2_14_BITS_RESOLUTION | CHARGER2_OPERATING_MODE) == 0)
        {
                printk("DEBUG_Charger : Initted I2c Charger successfully\r\n");
        }
        else
        {
               printk("DEBUG_charger :I2c Charger was not inited\r\n");        
        }

}

/**
 * @brief Reads voltage data from the I2C device.
 *
 * This function reads voltage data from the I2C registers of the device. It combines
 * the high and low bytes to form a 16-bit voltage data and then converts it to volts
 * using a conversion factor of 2.44. The voltage value is returned.
 *
 * @return float - Voltage read from the I2C device, in volts.
 */

float ReadI2CVoltage(void)
 {
    uint8_t ucVData1 = 0;
    uint8_t ucVData2 = 0;
    uint16_t usVData = 0;
    
    i2c_reg_read_byte(i2c_dev, SLAVE_ADDRESS, 0x01, &ucVData1);
    
    if ( ( ( ucVData1 & 0x08 )) )
    {
      ucVData1 = 0;

    // Read voltage data from I2C registers
    i2c_reg_read_byte(i2c_dev, SLAVE_ADDRESS, RegVoltageLowByte, &ucVData1);
    k_msleep(10);
    i2c_reg_read_byte(i2c_dev, SLAVE_ADDRESS, RegVoltageHighByte, &ucVData2);

    // Combine high and low bytes to form 16-bit voltage data
    usVData = ((uint16_t)ucVData2 << 8) | ucVData1;

    // Convert voltage data to volts (assuming a conversion factor of 2.44)
    float fVolt = (usVData * 2.44) / 1000.0;

    return fVolt;
    }
    else 
    {
        printk("DEBUG : PMIC is not ready for reading Voltage\n\r");
        // return 0;
    }
    
}


/**
 * @brief Reads temperature data from the I2C device.
 *
 * This function reads temperature data from the I2C registers of the device. It combines
 * the high and low bytes to form a 16-bit temperature data and then converts it to degrees
 * Celsius using a conversion factor of 0.125. The temperature value is returned.
 *
 * @return float - Temperature read from the I2C device, in degrees Celsius.
 */

float ReadI2CTemperature(void) 
{
    int8_t ucTData1 = 0;
    int8_t ucTData2 = 0;
    int16_t sTData = 0;

     i2c_reg_read_byte(i2c_dev, SLAVE_ADDRESS, 0x01, &ucTData1);
 
    
    if ((ucTData1 & 0x08))
    {
         // Read temperature data from I2C registers
    i2c_reg_read_byte(i2c_dev, SLAVE_ADDRESS, RegTemperatureLowByte, (uint8_t *)&ucTData1);
    k_msleep(10);
    printk("ucTData1:%d\r\n", ucTData1);
    i2c_reg_read_byte(i2c_dev, SLAVE_ADDRESS, RegTemperatureHighByte, (uint8_t *)&ucTData2);
    printk("ucTData2:%d\r\n", ucTData2);

    // Combine high and low bytes to form 16-bit temperature data
    sTData = ((int16_t)ucTData2 << 8) | (int8_t)ucTData1;

    // Check if temperature is negative
    // if (sTData & 0x8000 == 0) {
    //     // Convert from two's complement to signed integer
    //     sTData = -(~sTData + 1);
    // }

    sTData = (~(sTData - 0x01)) * -1;

    // Convert temperature data to degrees Celsius
    float fTemp = sTData * 0.125;

    return fTemp;
    }
    else
    {
        printk("DEBUG : PMIC is not ready for reading Temperature\n\r");
    }

   
}


 