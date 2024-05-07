/**
 * @file    AccelerometerHandler.h
 * @author : Devendu
 * @brief   File to get coordinates
 * @date    07-05-2024
 */
#ifndef _ACCELEROMETERHANDLER_H
#define _ACCELEROMETERHANDLER_H
 
/**************************************INCLUDES******************************/
#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
 
/***************************************MACROS*******************************/
typedef struct {
    short XAxis;
    short YAxis;
    short ZAxis;
} MC36XX_acc_t;
 
/**************************************FUNCTION DECLARATIONS****************/

bool GetID3630I2C(void);
MC36XX_acc_t MC3630readRawAccel(void);
#endif 
//EOF 