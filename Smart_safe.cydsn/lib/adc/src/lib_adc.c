/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
/**
 * @file lib_adc.c
 * @brief ADC driver implementation for channel initialization and reads.
 */

#include "lib_adc.h"
#include <project.h>

#define LOG_LEVEL LOG_LEVEL_INFO
#define TAG "ADC"
#include "log_dbg.h"

/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/
/**
 * @brief Initialize ADC block and start continuous conversion.
 */
void lib_adc_init()
{
    ADC_Start();
    ADC_StartConvert();
    LOG_I(TAG, "ADC initialized and conversion started");
}

/**
 * @brief Read a raw sample from selected ADC channel.
 *
 * @param channel Channel identifier from lib_adc_channel_num_t.
 * @return int16_t Raw ADC sample value.
 */
int16_t lib_adc_get(lib_adc_channel_num_t channel)
{
    int16_t ret_val = 0;
    switch(channel)
    {
        case ADC_CH_POT_2_0:
            ret_val = ADC_GetResult16(0);
            LOG_D(TAG, "Read ADC_CH_POT_2_0: %d", ret_val);
        break;
        case ADC_CH_POT_2_1:
            ret_val = ADC_GetResult16(1);
            LOG_D(TAG, "Read ADC_CH_POT_2_1: %d", ret_val);
        break;
        case ADC_CH_EXT_2_5:
            ret_val = ADC_GetResult16(2);
            LOG_D(TAG, "Read ADC_CH_EXT_2_5: %d", ret_val);
        break;
    }
    
    return ret_val;
}

/* [] END OF FILE */
