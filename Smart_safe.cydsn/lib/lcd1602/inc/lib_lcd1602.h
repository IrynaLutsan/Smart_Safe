/**
 * @file lib_lcd1602.h
 * @brief HD44780-compatible LCD display interface.
 */

#ifndef LIB_LCD1602_H
#define LIB_LCD1602_H

#include "stdint.h"

/**
 * @brief LCD backlight control states.
 */
typedef enum lib_lcd_bklight
{
    LCD_BACKLIGHT_OFF = 0, /**< Turn the LCD backlight off. */
    LCD_BACKLIGHT_ON       /**< Turn the LCD backlight on. */
} lib_lcd_bklight_t;

/**
 * @brief Initialize the LCD display.
 */
void lib_lcd1602_init(void);

/**
 * @brief Clear the LCD screen and reset the cursor.
 */
void lib_lcd1602_clear(void);

/**
 * @brief Write a string to the LCD at the specified position.
 *
 * @param col Starting column index.
 * @param row Starting row index.
 * @param str Null-terminated string to display.
 */
void lib_lcd1602_write_str(uint8_t col, uint8_t row, char* str);

/**
 * @brief Control the LCD backlight state.
 *
 * @param state Backlight state to apply.
 */
void lib_lcd1602_backlight(lib_lcd_bklight_t state);

#endif // LIB_LCD1602_H
    
/* [] END OF FILE */
