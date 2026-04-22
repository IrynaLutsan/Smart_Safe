/**
 * @file lib_magnetometer.h
 * @brief Magnetometer sensor interface for the smart safe project.
 */

#ifndef LIB_MAGNETOMETER_H
#define LIB_MAGNETOMETER_H

#include "lib_common_types.h"

/**
 * @brief Magnetometer sensor reading structure.
 */
typedef struct lib_magnetometer_data{
    struct {
        int32_t x; /**< Magnetometer X-axis raw data. */
        int32_t y; /**< Magnetometer Y-axis raw data. */
        int32_t z; /**< Magnetometer Z-axis raw data. */
    } mag;
    
    int16_t temperature; /**< Sensor temperature reading. */
    uint8_t is_new : 1;   /**< Indicates whether the data is new since the last read. */
} lib_magnetometer_data_t;
    
/**
 * @brief Initialize the magnetometer sensor interface.
 */
void lib_magnetometer_init(void);

/**
 * @brief Get the latest magnetometer sensor values.
 *
 * @return The current @ref lib_magnetometer_data_t values.
 */
lib_magnetometer_data_t lib_magnetometer_get(void);

#endif // LIB_MAGNETOMETER_H

/* [] END OF FILE */

