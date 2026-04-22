/**
 * @file lib_adc.h
 * @brief ADC channel interface for analog sensor readings.
 */

#ifndef LIB_ADC_H
#define LIB_ADC_H

#include "stdint.h"

/**
 * @brief ADC channel identifiers.
 */
typedef enum lib_adc_channel_num
{
    ADC_CH_POT_2_0 = 0, /**< Potentiometer channel 2.0. */
    ADC_CH_POT_2_1,     /**< Potentiometer channel 2.1. */
    ADC_CH_EXT_2_5,     /**< External analog input channel 2.5. */
} lib_adc_channel_num_t;

/**
 * @brief Initialize the ADC hardware.
 */
void lib_adc_init(void);

/**
 * @brief Read the value from the specified ADC channel.
 *
 * @param channel ADC channel to read.
 * @return Raw ADC sample value.
 */
int16_t lib_adc_get(lib_adc_channel_num_t channel);

#endif // LIB_ADC_H
/* [] END OF FILE */
