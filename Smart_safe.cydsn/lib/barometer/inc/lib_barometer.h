/**
 * @file lib_barometer.h
 * @brief Barometer sensor interface.
 */

#ifndef LIB_BAROMETER_H
#define LIB_BAROMETER_H

#include "lib_common_types.h"

/**
 * @brief Barometer sensor data structure.
 */
typedef struct lib_barometer_data{
    uint32_t pressure;   /**< Atmospheric pressure reading in Pascals. */
    int16_t temperature; /**< Temperature reading from the barometer sensor. */
} lib_barometer_data_t;

/**
 * @brief Initialize the barometer sensor interface.
 */
void lib_barometer_init(void);

/**
 * @brief Read the latest barometer sensor values.
 *
 * @return A @ref lib_barometer_data_t structure containing pressure and temperature.
 */
lib_barometer_data_t lib_barometer_get(void);

#endif // LIB_BAROMETER_H

/* [] END OF FILE */
