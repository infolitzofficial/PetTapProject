/**
 * @file    Temp_Adchandler.c
 * @author : Devendu
 * @brief   File to calculate the temperature using MCP9700
 * @date    09-04-2023
 */
/***********************************INCLUDES**************************/
#include <nrfx_example.h>
#include <saadc_examples_common.h>
#include <nrfx_saadc.h>
#include <zephyr/kernel.h>
#include "TempAdcHandler.h"


/************************************MACROS***************************/
/** @brief Symbol specifying analog input to be observed by SAADC channel 0. */
#define CH0_AIN ANALOG_INPUT_TO_SAADC_AIN(ANALOG_INPUT_A0)

// Temperature coefficient in mV/°C
#define TC_MV_PER_C 10.0

// Output voltage at 0°C in mV
#define V0_C_MV 440


/************************************GLOBALS**************************/


/**********************************FUNCTION DEFINITIONS****************/

/**
 * @brief Converts raw ADC value to temperature using the MCP9700 formula.
 *
 * This function takes a raw ADC value as input and converts it to temperature using the MCP9700 formula. 
   It assumes a 10-bit resolution and a reference voltage of 3.3V.
 *
 * @param adc_value The raw ADC value to be converted to temperature.
 * @return float - Temperature calculated from the ADC value, in degrees Celsius.
 */

static float AdcToTemperature(uint16_t iAdcValue) 
{
    // Convert ADC value to voltage (assuming 10-bit resolution and Vref = 3.3V)
    float fVoltage,fTemperature;
    fVoltage= (float)iAdcValue * 3.3 / 1024.0;

    // Calculate temperature using MCP9700 formula
    fTemperature= (fVoltage - V0_C_MV / 1000.0) / (TC_MV_PER_C / 1000.0);
    return fTemperature;
}

/**
 * @brief Initializes the SAADC module.
 *
 * This function initializes the SAADC module with default configurations and sets up a single SAADC channel for sampling.
 */

 void InitializeSaadc(void)
{
    /** @brief SAADC channel configuration structure for single channel use. */
    static const nrfx_saadc_channel_t m_single_channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(CH0_AIN, 0);
    nrfx_err_t status;
    // m_single_channel= NRFX_SAADC_DEFAULT_CHANNEL_SE(CH0_AIN, 0);

    status = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    status = nrfx_saadc_channel_config(&m_single_channel);
    NRFX_ASSERT(status == NRFX_SUCCESS);
    
    uint32_t channels_mask = nrfx_saadc_channels_configured_get();
    status = nrfx_saadc_simple_mode_set(channels_mask,
                                        NRF_SAADC_RESOLUTION_10BIT,
                                        NRF_SAADC_OVERSAMPLE_DISABLED,
                                        NULL);
    NRFX_ASSERT(status == NRFX_SUCCESS);
}

/**
 * @brief Reads raw ADC value from the SAADC.
 *
 * This function triggers SAADC sampling, reads the raw ADC value, and returns it.
 *
 * @return uint16_t - Raw ADC value sampled by the SAADC.
 */

uint16_t AnalogRead(void)
{
    uint16_t iSampleValue;
    nrfx_err_t status;
  
    status = nrfx_saadc_buffer_set(&iSampleValue, 1);
    NRFX_ASSERT(status == NRFX_SUCCESS);
    status = nrfx_saadc_mode_trigger();
    NRFX_ASSERT(status == NRFX_SUCCESS);

    return iSampleValue;
}

/**
 * @brief Calculates temperature from the raw ADC value.
 *
 * This function reads the raw ADC value using AnalogRead(), which returns a 16-bit unsigned integer.
 * If the ADC reading is successful (i.e., not equal to 0xFFFF), it converts the raw ADC value to temperature
 * using the adc_to_temperature() function.
 * The temperature value is then stored at the memory location pointed to by the provided pointer pfTemperature.
 * If the ADC reading fails, the temperature value is set to 0.0°C.
 *
 * @param pfTemperature Pointer to a float variable where the calculated temperature will be stored.
 *                     If the ADC reading fails, the temperature value will be set to 0.0°C.
 */

void CalculateTemperature(float *pfTemperature) 
{
    uint16_t iSampleValue;
    float fTemperature = 0.0f; // Initialize to 0 in case of failure

    iSampleValue = AnalogRead();

    // Check if AnalogRead fails
    if (iSampleValue != 0xFFFF) {
        fTemperature = AdcToTemperature(iSampleValue);
    }

    // Store the result at the provided address
    *pfTemperature = fTemperature;

    printk("Temperature........: %.2f°C\r\n", fTemperature);
}

/** @} */
