/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "lib_lcd1602.h"
#include "liquidC.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#define TAG "LCD"
#include "log_dbg.h"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
*********************************************************************************/
#define LCD1602_ADDR 0x24u
#define LCD1602_COL_CNT 16u
#define LCD1602_ROW_CNT 2u


/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/
/**
 * @brief   Initialize LCD1602 display via I2C
 * @details Initializes the LCD1602 display with I2C address 0x24, 16 columns and 2 rows.
 *          Performs backlight blinking sequence (3 times) to indicate initialization,
 *          then turns backlight on permanently
 * @return  void
 * @note    Must be called before using other LCD functions. Uses 100ms delays for blinking
 */
void lib_lcd1602_init(void)
{
   LCDI2C_init(LCD1602_ADDR,LCD1602_COL_CNT,LCD1602_ROW_CNT);
   LOG_D(TAG, "LCD1602 I2C init: addr=0x%02X, cols=%d, rows=%d", LCD1602_ADDR, LCD1602_COL_CNT, LCD1602_ROW_CNT);
   
   for(int i = 0; i< 3; i++)
   {
        LCDI2C_backlight();
        CyDelay(100);
        LCDI2C_noBacklight();
        CyDelay(100);
   }
   LCDI2C_backlight();
   LOG_I(TAG, "LCD1602 initialized with backlight test sequence");
}

/**
 * @brief   Clear LCD1602 display
 * @details Clears all characters from the LCD display and returns cursor to home position
 * @return  void
 * @note    This function clears the display content but keeps backlight state unchanged
 */
void lib_lcd1602_clear()
{
    LCDI2C_clear();
    LOG_D(TAG, "Display cleared");
}

/**
 * @brief   Write string to LCD1602 at specified position
 * @param   col         Column position (0-15 for 16-column display)
 * @param   row         Row position (0-1 for 2-row display)
 * @param   str         Null-terminated string to write
 * @details Sets cursor to specified column and row, then writes the string.
 *          String will be truncated if it exceeds display width
 * @return  void
 * @note    Column 0 is leftmost, Row 0 is top row. Invalid positions are handled by LCDI2C_setCursor
 */
void lib_lcd1602_write_str(uint8_t col, uint8_t row, char* str)
{
    LCDI2C_setCursor(col,row);
    LCDI2C_write_String(str);
    LOG_D(TAG, "Write at (%d,%d): %s", col, row, str);
}

/**
 * @brief   Control LCD1602 backlight state
 * @param   state       Backlight state (LCD_BACKLIGHT_ON or LCD_BACKLIGHT_OFF)
 * @details Controls the LCD backlight by calling appropriate LCDI2C functions
 *          based on the requested state
 * @return  void
 * @note    Invalid state values are ignored. Backlight state persists until changed
 */
void lib_lcd1602_backlight(lib_lcd_bklight_t state)
{
    switch(state)
    {
        case LCD_BACKLIGHT_ON:
            LCDI2C_backlight();
            LOG_D(TAG, "Backlight ON");
        break;
        case LCD_BACKLIGHT_OFF:
            LCDI2C_noBacklight();
            LOG_D(TAG, "Backlight OFF");
        break;
        default:
        break;
    }
}


/* [] END OF FILE */
