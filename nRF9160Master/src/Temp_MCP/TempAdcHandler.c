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
#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>

/** @brief Symbol specifying analog input to be observed by SAADC channel 0. */
#define CH0_AIN ANALOG_INPUT_TO_SAADC_AIN(ANALOG_INPUT_A0)

// Temperature coefficient in mV/°C
#define TC_MV_PER_C 10.0

// Output voltage at 0°C in mV
#define V0_C_MV 440


/************************************GLOBALS**************************/

/** @brief Array specifying GPIO pins used to test the functionality of SAADC. */
static uint8_t m_out_pins[1] = {LOOPBACK_PIN_1B, LOOPBACK_PIN_2B, LOOPBACK_PIN_3B};

/** @brief Samples buffer defined with the size of @ref CHANNEL_COUNT symbol to store values from each channel ( @ref m_multiple_channels). */
static nrf_saadc_value_t samples_buffer[1];

/** @brief SAADC channel configuration structure for single channel use. */
static const nrfx_saadc_channel_t m_single_channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(CH0_AIN, 0);



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

static float adc_to_temperature(uint16_t adc_value) {
    // Convert ADC value to voltage (assuming 10-bit resolution and Vref = 3.3V)
    float voltage = (float)adc_value * 3.3 / 1024.0;

    // Calculate temperature using MCP9700 formula
    float temperature = (voltage - V0_C_MV / 1000.0) / (TC_MV_PER_C / 1000.0);
    return temperature;
}

/**
 * @brief Initializes the SAADC module.
 *
 * This function initializes the SAADC module with default configurations and sets up a single SAADC channel for sampling.
 */

 void initialize_saadc(void) {
    nrfx_err_t status;
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
    nrfx_err_t status;
    uint16_t sample_value;

    status = nrfx_saadc_buffer_set(&sample_value, 1);
    NRFX_ASSERT(status == NRFX_SUCCESS);
    status = nrfx_saadc_mode_trigger();
    NRFX_ASSERT(status == NRFX_SUCCESS);

    return sample_value;
}

/**
 * @brief Calculates temperature from the raw ADC value.
 *
 * This function reads the raw ADC value using AnalogRead(), converts it to temperature using the adc_to_temperature() function,
   and logs the temperature.
 */

 void calculate_Temperature(void) {
    uint16_t sample_value=AnalogRead();

    float temperature = adc_to_temperature(sample_value);
    printk("Temperature: %.2f°C\r\n", temperature);
}
/** @} */
