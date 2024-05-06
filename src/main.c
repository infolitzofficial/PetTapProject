#include "zephyr/sys/printk.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/_stdint.h>
#include <zephyr/kernel.h>

// #include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

typedef struct {
    short XAxis;
    short YAxis;
    short ZAxis;
} MC36XX_acc_t;

static const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c2));
#define MC36XX_CFG_I2C_ADDR                   0x4C
#define MC36XX_REG_MODE_CNTRL                 0x10
#define MC36XX_MODE_CNTRL_CWAKE_VALUE         0x05
#define MC36XX_REG_PROID                      0x18
#define MC36XX_REG_XOUT_LSB                   0x02 

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

MC36XX_acc_t MC3630readRawAccel(void)
{

    MC36XX_acc_t AccRaw; 
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

int main(void)
{
    int8_t nRetVal = 0;
    int8_t ucData1 = 0;
    int8_t ucData2 = 0;
    int16_t usData = 0;
        
    GetID3630I2C();

    while (true)
    {   printk("DEBUG:Reading the accelerometer values\r\n");
    /*  ucData1 = 0;
                i2c_reg_read_byte(i2c_dev, 0x4C, 0x02, &ucData1);
                // printk("DEBUG : ucData 1 %d\n\r", ucData1);
                k_msleep(10);
                i2c_reg_read_byte(i2c_dev, 0x4C, 0x03, &ucData2);

                // printk("DEBUG : ucData 2 %d\n\r", ucData2);

                usData = ((uint16_t)ucData2 << 8) | (ucData1);
                printk("DEBUG : X axis -- %d\n\r", usData);
                ucData1 = 0;

                i2c_reg_read_byte(i2c_dev, 0x4C, 0x04, &ucData1);
                // printk("DEBUG : ucData 1 %d\n\r", ucData1);
                k_msleep(10);
                i2c_reg_read_byte(i2c_dev, 0x4C, 0x05, &ucData2);

                // printk("DEBUG : ucData 2 %d\n\r", ucData2);

                usData = ((uint16_t)ucData2 << 8) | (ucData1);
                printk("DEBUG : Y axis -- %d\n\r", usData);
                ucData1 = 0;


                i2c_reg_read_byte(i2c_dev, 0x4C, 0x06, &ucData1);
                // printk("DEBUG : ucData 1 %d\n\r", ucData1);
                k_msleep(10);
                i2c_reg_read_byte(i2c_dev, 0x4C, 0x07, &ucData2);

                // printk("DEBUG : ucData 2 %d\n\r", ucData2);

                usData = ((uint16_t)ucData2 << 8) | (ucData1);
                printk("DEBUG : Z axis -- %d\n\r", usData);
                ucData1 = 0;*/
    
    MC36XX_acc_t rawAccel = MC3630readRawAccel();
    k_msleep(10); 
    printk("X axis: %d\n", rawAccel.XAxis);
    printk("Y axis: %d\n", rawAccel.YAxis);
    printk("Z axis: %d\n", rawAccel.ZAxis);
    k_msleep(1000); // Sleep for 1 second
    }
}
