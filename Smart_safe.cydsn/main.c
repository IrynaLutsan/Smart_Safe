/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "project.h"
#include "lib_mkb.h"
#include "lib_seg_display.h"
#include "lib_lcd1602.h"
#include "lib_adc.h"
#include "lib_acc_gyr.h"
#include "lib_magnetometer.h"
#include "lib_barometer.h"
#include "lib_rfid.h"
#include "lib_common_types.h"
#include "lib_buzzer.h"
#include "lib_servo.h"
#include <stdio.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#define TAG "MAIN"
#include "log_dbg.h"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
 *********************************************************************************/


/********************************************************************************
 **********                         PRIVATE VARIABLES                 ***********
*********************************************************************************/


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/


/********************************************************************************
 **********                             MAIN                          ***********
*********************************************************************************/
int main(void)
{
    CyGlobalIntEnable;

    dbg_log_init();
    LOG_I(TAG, "TEST");

    I2C_Start();
    lib_lcd1602_init();
    lib_lcd1602_clear();
    lib_lcd1602_write_str(0u, 0u, "TEST");

    for(;;)
    {
        LED_GREEN_Write(1u);
        CyDelay(500u);
        LED_GREEN_Write(0u);
        CyDelay(500u);
    }
}

/* [] END OF FILE */
