/**
 * @file lib_mkb.h
 * @brief Matrix keypad interface for the smart safe project.
 */

#ifndef LIB_MKB_H
#define LIB_MKB_H

#include "lib_common_types.h"

/**
 * @brief Keypad status codes returned by @ref lib_mkb_read.
 */
typedef enum lib_mkb_status
{
    LIB_MKB_NO_EVENTS = 0, /**< No keypad state change detected. */
    LIB_MKB_STATE_CHANGED, /**< Keypad state changed since last scan. */
} lib_mkb_status_t;

/**
 * @brief Matrix keypad button codes.
 */
typedef enum lib_mkb_key
{
    LIB_MKB_KEY_0 = 0,
    LIB_MKB_KEY_1 = 1,
    LIB_MKB_KEY_2 = 2,
    LIB_MKB_KEY_3 = 3,
    LIB_MKB_KEY_4 = 4,
    LIB_MKB_KEY_5 = 5,
    LIB_MKB_KEY_6 = 6,
    LIB_MKB_KEY_7 = 7,
    LIB_MKB_KEY_8 = 8,
    LIB_MKB_KEY_9 = 9,
    LIB_MKB_KEY_STAR = 10,
    LIB_MKB_KEY_HASH = 11,
} lib_mkb_key_t;

/**
 * @brief Key transition state for a keypad event.
 */
typedef enum lib_mkb_key_state
{
    LIB_MKB_KEY_RELEASED = 0,
    LIB_MKB_KEY_PRESSED = 1,
} lib_mkb_key_state_t;

/**
 * @brief Keypad read result with status and pressed key code.
 */
typedef struct lib_mkb_result
{
    lib_mkb_status_t status; /**< Current scan status. */
    lib_mkb_key_t key_code;  /**< Pressed key code (0..9, star, hash). */
    lib_mkb_key_state_t key_state; /**< Key transition state (pressed/released). */
} lib_mkb_result_t;

/**
 * @brief Initialize the matrix keypad hardware interface.
 *
 * Configures keypad columns as high-impedance inputs and prepares
 * internal state for the first scan.
 */
void lib_mkb_init(void);

/**
 * @brief Read the current matrix keypad state.
 *
 * Scans all columns and rows, detects state changes, and copies the
 * raw row/column values into the provided buffer.
 *
 * @param[out] keys Raw keypad matrix state buffer [4][3].
 *
 * @return Keypad read result containing status, key code, and key state.
 */
lib_mkb_result_t lib_mkb_read(uint8_t keys[4][3]);

#endif // LIB_MKB_H

/* [] END OF FILE */
