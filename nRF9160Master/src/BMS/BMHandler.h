#ifndef _I2CCHARGERHANDLER_H
#define _I2CCHARGERHANDLER_H
 
/**************************************INCLUDES******************************/
#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
 
/***************************************MACROS*******************************/

 
/**************************************FUNCTION DECLARATIONS****************/
void InitI2CCharger(void);
float ReadI2CVoltage(void);
float ReadI2CTemperature(void); 
#endif 
//EOF 