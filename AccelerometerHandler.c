 /* @file    AccelerometerHandler.c
 * @author  
 * @date    04 May 2024
 * @brief   Driver interface header file for accelerometer mc36xx series.
 * @see     http://www.mcubemems.com
 */

/***********************************INCLUDES**************************/

#include "zephyr/sys/printk.h"
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include<AccelerometerHandler.h>



