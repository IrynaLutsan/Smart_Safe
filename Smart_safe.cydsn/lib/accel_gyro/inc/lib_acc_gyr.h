/**
 * @file lib_acc_gyr.h
 * @brief Accelerometer and gyroscope sensor interface.
 */

#ifndef LIB_ACC_GYR_H
#define LIB_ACC_GYR_H

#include "lib_common_types.h"

/**
 * @brief Combined accelerometer, gyroscope, and temperature data.
 */
typedef struct lib_acc_gyr_data {
    struct {
        uint8_t acc : 1; /**< New accelerometer data available. */
        uint8_t gyr : 1; /**< New gyroscope data available. */
        uint8_t tmp : 1; /**< New temperature data available. */
    } is_new;
    
    struct {
        int32_t x; /**< Accelerometer X-axis value. */
        int32_t y; /**< Accelerometer Y-axis value. */
        int32_t z; /**< Accelerometer Z-axis value. */
    } acc;
    
    struct {
        int32_t x; /**< Gyroscope X-axis value. */
        int32_t y; /**< Gyroscope Y-axis value. */
        int32_t z; /**< Gyroscope Z-axis value. */
    } gyr;
    
    int16_t temperature; /**< Temperature reading in degrees C. */
} lib_acc_gyr_data_t;
    
/**
 * @brief Initialize the accelerometer and gyroscope sensor interface.
 */
void lib_acc_gyr_init(void);

/**
 * @brief Get the latest accelerometer, gyroscope, and temperature data.
 *
 * @return The current @ref lib_acc_gyr_data_t values.
 */
lib_acc_gyr_data_t lib_acc_gyr_get(void);

#endif // LIB_ACC_GYR_H

/* [] END OF FILE */
