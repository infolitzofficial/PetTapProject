/**
 * @file    Temp_Adchandler.h
 * @author : Devendu
 * @brief   File to calculate the temperature using MCP9700
 * @date    09-04-2023
 * @see      Temp_Adchandler.c

 */

#ifndef _TEMP_ADC_HANDLER_H
#define _TEMP_ADC_HANDLER_H
 
/**************************************INCLUDES******************************/
#include <stddef.h>
#include <stdint.h>
#include <saadc_examples_common.h>
#include <nrfx_saadc.h>
#include <zephyr/kernel.h>
 
/***************************************MACROS*******************************/

 
/**************************************FUNCTION DECLARATIONS****************/
static float AdcToTemperature(uint16_t iAdcValue);
void InitializeSaadc(void) ;
uint16_t AnalogRead(void);
void CalculateTemperature(float *pfTemperature);
 
#endif 
//EOF