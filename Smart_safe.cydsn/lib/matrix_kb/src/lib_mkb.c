/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "project.h"
#include "lib_mkb.h"
#include <stdio.h>
#include <string.h>

#define LOG_LEVEL LOG_LEVEL_INFO
#include "log_dbg.h"
#define TAG "MKB"

/********************************************************************************
 **********                         PRIVATE VARIABLES                 ***********
*********************************************************************************/
static uint8_t keys[4][3];
static uint8_t keys_last[4][3];
static uint8_t keys_act[4][3] = {
    { LIB_MKB_KEY_1, LIB_MKB_KEY_2, LIB_MKB_KEY_3 },
    { LIB_MKB_KEY_4, LIB_MKB_KEY_5, LIB_MKB_KEY_6 },
    { LIB_MKB_KEY_7, LIB_MKB_KEY_8, LIB_MKB_KEY_9 },
    { LIB_MKB_KEY_STAR, LIB_MKB_KEY_0, LIB_MKB_KEY_HASH }
};


static void (*COLx_SetDriveMode[3])(uint8_t mode) = {
    MKB_COL0_SetDriveMode,
    MKB_COL1_SetDriveMode,
    MKB_COL2_SetDriveMode};

static void (*COLx_Write[3])(uint8_t value) = {
    MKB_COL0_Write,
    MKB_COL1_Write,
    MKB_COL2_Write};


static uint8 (*ROWx_Read[4])() = {
    MKB_ROW0_Read,
    MKB_ROW1_Read,
    MKB_ROW2_Read,
    MKB_ROW3_Read};

enum
{
    MKB_ROW_COUNT = (int)(sizeof(keys_act) / sizeof(keys_act[0])),
    MKB_COL_COUNT = (int)(sizeof(keys_act[0]) / sizeof(keys_act[0][0]))
};

/********************************************************************************
 **********                        FUNCTION PROTOTYPES                ***********
*********************************************************************************/

/**
 * @brief Print the internal keypad matrix state to the debug log.
 *
 * This function is used for diagnostics when the keypad state changes.
 */
static void print_matrix(void);


/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief Initialize the matrix keypad hardware interface.
 *
 * Configures each column pin as high impedance and initializes the last
 * known key state for debounce and event detection.
 */
void lib_mkb_init(void) 
{
    LOG_I(TAG, "Initializing matrix keypad...");
    for (int i = 0 ; i < MKB_COL_COUNT; i++)
    {
        COLx_SetDriveMode[i](MKB_COL0_DM_DIG_HIZ);
        LOG_D(TAG, "Column %d configured as high impedance", i);
    }
    memset(keys_last, 1, sizeof(keys));
    
    LOG_I(TAG, "Matrix keypad initialization complete");
}


/**
 * @brief Scan the keypad matrix and detect key state changes.
 *
 * Iterates through each column, drives it low, reads row inputs, and
 * compares the result to the previous scan. If any key state has changed,
 * the raw matrix data is copied to the provided buffer.
 *
 * @param[out] keys_raw Output buffer to receive the raw [4][3] keypad state.
 *
 * @return Keypad read result containing status, key code, and key state.
 */
lib_mkb_result_t lib_mkb_read(uint8_t keys_raw[4][3]) 
{
    lib_mkb_result_t result = {
        .status = LIB_MKB_NO_EVENTS,
        .key_code = LIB_MKB_KEY_0,
        .key_state = LIB_MKB_KEY_RELEASED,
    };

    for (uint8_t colIndex = 0; colIndex < MKB_COL_COUNT; colIndex++) 
    {
        // col: set to output to low
        //uint8_t curCol = cols[colIndex];

        COLx_SetDriveMode[colIndex](MKB_COL0_DM_STRONG);
        COLx_Write[colIndex](0);

        // row: interate through the rows
        for (int rowIndex=0; rowIndex < MKB_ROW_COUNT; rowIndex++) 
        {
            //uint8_t rowCol = rows[rowIndex];
            //pinMode(rowCol, INPUT_PULLUP);
            keys[rowIndex][colIndex] = ROWx_Read[rowIndex]();
            if (keys[rowIndex][colIndex] != keys_last[ rowIndex ][colIndex])
            {
                print_matrix();
                lib_mkb_key_t key_code = (lib_mkb_key_t)keys_act[rowIndex][colIndex];
                lib_mkb_key_state_t key_state = (keys[rowIndex][colIndex] == 0u)
                    ? LIB_MKB_KEY_PRESSED
                    : LIB_MKB_KEY_RELEASED;
                LOG_D(TAG, "Key event: Row=%d Col=%d Code=%d State=%d", rowIndex, colIndex, key_code, key_state);
                memcpy(keys_raw, keys, sizeof(keys));
                result.status = LIB_MKB_STATE_CHANGED;
                result.key_code = key_code;
                result.key_state = key_state;
                keys_last[rowIndex][colIndex] = keys[rowIndex ][colIndex];
                COLx_SetDriveMode[colIndex](MKB_COL0_DM_DIG_HIZ);
                return result;
            }
            keys_last[rowIndex][colIndex] = keys[rowIndex][colIndex];
        }
        // disable the column
        COLx_SetDriveMode[colIndex](MKB_COL0_DM_DIG_HIZ);
    }
    return result;
}


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/
static void print_matrix(void)
{
    char matrix_str[64] = "";
    char temp[8];
    
    for(int i = 0; i < 4; i ++)
    {
        for(int j = 0; j < 3; j++)
        {
            sprintf(temp, "%02X ", keys[i][j]);
            strcat(matrix_str, temp);
        }
        if(i < 3) {
            strcat(matrix_str, "| ");
        }
    }
    
    LOG_D(TAG, "Key matrix: %s", matrix_str);
}

/* [] END OF FILE */
