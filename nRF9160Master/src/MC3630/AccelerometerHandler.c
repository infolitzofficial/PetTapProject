/**
 * @file    AccelerometerHandler.c
 * @author : Devendu
 * @brief   File to get coordinates
 * @date    07-05-2024
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
#include "AccelerometerHandler.h"

/************************************MACROS***************************/
#define MC36XX_CFG_I2C_ADDR                   0x4C
#define MC36XX_REG_MODE_CNTRL                 0x10
#define MC36XX_MODE_CNTRL_CWAKE_VALUE         0x05
#define MC36XX_REG_PROID                      0x18
#define MC36XX_REG_XOUT_LSB                   0x02 
/************************************GLOBALS**************************/


static const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c3));
MC36XX_acc_t AccRaw;

/**********************************FUNCTION DEFINITIONS****************/

/**
 * @brief Initializes the I2C communication with the MC3630 accelerometer device and checks for device detection.
 *
 * This function initializes the I2C communication with the MC3630 accelerometer device 
 * by writing specific configuration data to the appropriate register of the device. 
 * It checks for successful initialization and prints debug messages accordingly.
 *
 * @return bool - Returns true if the device is detected, false otherwise.
 */


bool GetID3630I2C(void)
{
    int8_t nRetVal = 0;
    uint8_t id=0;
    nRetVal = i2c_reg_write_byte(i2c_dev, MC36XX_CFG_I2C_ADDR, MC36XX_REG_MODE_CNTRL, MC36XX_MODE_CNTRL_CWAKE_VALUE); 
    if(nRetVal)
    {
        printk("DEBUG_MC3630 : Init I2c Failed with err : %d\r\n", nRetVal);
        return false;
    }
          
    i2c_reg_read_byte(i2c_dev, MC36XX_CFG_I2C_ADDR, MC36XX_REG_PROID, &id);
    if (id != 0x71)
    {
        printk("No MC36XX detected!");
        printk("DEBUG_MC3630 : Getting ID is : %02X\r\n", id);  
        return false;
    }
    else
    {
        printk("MC36XX detected!\r\n"); 
        return true;  
    }
}

/**
 * @brief Reads raw accelerometer data from the MC3630 device.
 *
 * This function reads raw accelerometer data from the MC3630 device by performing 
 * a burst read operation from the appropriate registers. It combines the high and low 
 * bytes to form 16-bit accelerometer data for each axis and stores the result in 
 * a structure. The structure containing raw X, Y, and Z axis accelerometer data is returned.
 *
 * @return MC36XX_acc_t - Structure containing raw X, Y, and Z axis accelerometer data.
 */

MC36XX_acc_t MC3630readRawAccel(void)
{

    uint8_t rawData[6];
    // Read the six raw data registers into data array
    
    i2c_burst_read(i2c_dev,MC36XX_CFG_I2C_ADDR,MC36XX_REG_XOUT_LSB, rawData, 6);
    short x = (short)((((unsigned short)rawData[1]) << 8) | rawData[0]);
    short y = (short)((((unsigned short)rawData[3]) << 8) | rawData[2]);
    short z = (short)((((unsigned short)rawData[5]) << 8) | rawData[4]);

    AccRaw.XAxis = (short) (x);
    AccRaw.YAxis = (short) (y);
    AccRaw.ZAxis = (short) (z);
    return AccRaw;
}

/**
 * @brief Determines pet movement based on accelerometer readings.
 *
 * This function checks if there's any change in accelerometer readings compared to the previous readings.
 * If there's a change in any axis (X, Y, or Z), it indicates pet movement; otherwise, it indicates no movement.
 *
 * @param PreAccRaw The previous accelerometer readings.
 * @return int Returns 1 if pet movement is detected, 0 otherwise.
 */
int PetMove(MC36XX_acc_t PreAccRaw)
{   
    
 	// MC36XX_acc_t rawAccel = MC3630readRawAccel();
    MC3630readRawAccel();
    k_msleep(10);

    // printk("DEBUG : Previous value Xaxis %d\n\r", PreAccRaw.XAxis); 
    if(PreAccRaw.XAxis!=AccRaw.XAxis||PreAccRaw.YAxis!=AccRaw.YAxis||PreAccRaw.ZAxis!=AccRaw.ZAxis)
    {
    /*printk("Previous value X axis: %d, Current value X axis: %d \n", PreAccRaw.XAxis, AccRaw.XAxis);
     printk("Previous value Y axis: %d, Current value Y axis: %d\n", PreAccRaw.YAxis, AccRaw.YAxis);
     printk("Previous value Z axis: %d, Current value Z axis: %d\n", PreAccRaw.ZAxis, AccRaw.ZAxis);*/
     printk("DEBUG:Pet is moving\r\n");
     return 1;
    }
    else
    {
     printk("DEBUG:Pet is not moving\r\n");
     return 0;
    }

}
/**
 * @brief Retrieves a pointer to the raw accelerometer data.
 *
 * This function returns a pointer to the structure containing the most recent raw accelerometer data
 * for X, Y, and Z axes.
 *
 * @return MC36XX_acc_t* Pointer to the structure containing raw accelerometer data.
 */
MC36XX_acc_t *GetMC36Data()
{
    return &AccRaw;
}