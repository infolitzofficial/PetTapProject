#ifndef _I2CCHARGERHANDLER_H
#define _I2CCHARGERHANDLER_H
 
/**************************************INCLUDES******************************/
#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
 
/***************************************MACROS*******************************/

 
/**************************************FUNCTION DECLARATIONS****************/
void InitI2CCharger(void);
float ReadI2CPMIC(float *pfVolt, float *pfTemp); 
#endif 
//EOF 