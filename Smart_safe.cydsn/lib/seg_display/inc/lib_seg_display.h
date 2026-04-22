/**
 * @file lib_seg_display.h
 * @brief 7-segment display interface based on 74HC595 shift registers.
 */

#ifndef LIB_SEG_DISPLAY_H
#define LIB_SEG_DISPLAY_H

#include "project.h"
#include "stdint.h"

/**
 * @brief Number of digits handled by the display refresh buffer.
 */
#define LIB_SEG_DISPLAY_DIGITS_COUNT (8u)

/**
 * @brief Initialize the 7-segment display driver.
 */
void lib_seg_display_init(void);

/**
 * @brief Update the display refresh buffer.
 *
 * @param digits Pointer to an array of @ref LIB_SEG_DISPLAY_DIGITS_COUNT values.
 */
void lib_seg_display_update(const uint8_t *digits);

/**
 * @brief Update a single digit in the display refresh buffer.
 *
 * @param index Digit index in range 0..(@ref LIB_SEG_DISPLAY_DIGITS_COUNT - 1).
 * @param value Digit value to store for multiplexing.
 */
void lib_seg_display_update_digit(uint8_t index, uint8_t value);

/**
 * @brief Digit code for a blank (all segments off). Pass as value to
 *        lib_seg_display_update_digit() or fill a buffer with this value.
 */
#define SEG_DIGIT_BLANK  11u

/**
 * @brief Blank all 8 digits (all segments off).
 */
void lib_seg_display_clear(void);

/**
 * @brief Timer ISR used for display multiplexing.
 */
CY_ISR_PROTO(isr_7segment_interrupt_handler);

#endif // LIB_SEG_DISPLAY_H

/* [] END OF FILE */
