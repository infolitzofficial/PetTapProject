/**
 * @file    I2Cchargerhandler.h
 * @author : Devendu
 * @brief   File to get battery percentage
 * @date    25-04-2023
 */

#ifndef _I2CCHARGERHANDLER_H
#define _I2CCHARGERHANDLER_H
 
/**************************************INCLUDES******************************/
#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
 
/***************************************MACROS*******************************/

 
/**************************************FUNCTION DECLARATIONS****************/
void InitI2CCharger(void);
float ReadI2CPMIC(uint16_t *puPercent, float *pfTemp);
//float ReadI2CCurrent(void);
#endif 
//EOF 