/* ========================================
 * Driver refactor test
 * K1 = servo closed (0 deg)
 * K2 = servo open   (90 deg)
 * K3 = play melody
 * K4 = servo open + melody simultaneously
 * ======================================== */

/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "project.h"
#include "lib_mkb.h"
#include "lib_lcd1602.h"
#include "lib_buzzer.h"
#include "lib_servo.h"
#include <stdio.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#define TAG "MAIN"
#include "log_dbg.h"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
*********************************************************************************/

#define SERVO_TICK_MS  10u  /* step servo every 10 ms */


/********************************************************************************
 **********                         PRIVATE VARIABLES                 ***********
*********************************************************************************/

static volatile uint32_t g_ms = 0u;


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

static void systick_cb(void)
{
    g_ms++;
}


/********************************************************************************
 **********                             MAIN                          ***********
*********************************************************************************/
int main(void)
{
    CyGlobalIntEnable;

    dbg_log_init();

    /* Millisecond counter via SysTick */
    CySysTickStart();
    CySysTickSetCallback(0u, systick_cb);

    I2C_Start();
    lib_lcd1602_init();
    lib_lcd1602_clear();

    lib_mkb_init();
    lib_servo_init();   /* snaps to 0 deg (closed) */
    lib_buzzer_init();

    lib_lcd1602_write_str(0u, 0u, "K1:CLS  K2:OPEN");
    lib_lcd1602_write_str(0u, 1u, "K3:BUZ  K4:ALL ");

    LOG_I(TAG, "Driver test ready. Servo=0deg, Buzzer=idle");

    uint32_t servo_last_ms = 0u;
    uint32_t led_last_ms   = 0u;
    uint32_t lcd_last_ms   = 0u;
    uint8_t  led_state     = 0u;

    for (;;)
    {
        uint32_t now = g_ms;

        /* Advance servo one degree every 10 ms */
        if (now - servo_last_ms >= SERVO_TICK_MS)
        {
            lib_servo_tick();
            servo_last_ms = now;
        }

        /* Advance buzzer note-sequence state machine */
        lib_buzzer_tick(now);

        /* ---- NON-BLOCKING PROOF ----------------------------------------- */

        /* LED heartbeat: toggles every 500 ms regardless of servo/buzzer state.
           If it stops blinking, something is blocking the loop. */
        if (now - led_last_ms >= 500u)
        {
            led_state ^= 1u;
            LED_GREEN_Write(led_state);
            led_last_ms = now;
        }

        /* Live LCD status: row 0 shows servo angle + buzzer activity every 100 ms.
           Watch the angle count up/down while melody plays — both move together. */
        if (now - lcd_last_ms >= 100u)
        {
            char buf[17];
            snprintf(buf, sizeof(buf), "Srv:%3u %s",
                     (unsigned)lib_servo_get_angle(),
                     lib_buzzer_is_busy() ? "BUZ:ON " : "BUZ:-- ");
            lib_lcd1602_write_str(0u, 0u, buf);
            lcd_last_ms = now;
        }

        /* ------------------------------------------------------------------- */

        /* Keypad scan */
        uint8_t keys[4][3];
        lib_mkb_result_t kb = lib_mkb_read(keys);
        if (kb.status == LIB_MKB_STATE_CHANGED && kb.key_state == LIB_MKB_KEY_PRESSED)
        {
            switch (kb.key_code)
            {
                case LIB_MKB_KEY_1:
                    lib_servo_set_target(SERVO_CLOSED_DEG);
                    lib_lcd1602_write_str(0u, 1u, "Servo -> CLOSED");
                    LOG_I(TAG, "K1: servo -> 0 deg");
                    break;

                case LIB_MKB_KEY_2:
                    lib_servo_set_target(SERVO_OPEN_DEG);
                    lib_lcd1602_write_str(0u, 1u, "Servo -> OPEN  ");
                    LOG_I(TAG, "K2: servo -> 90 deg");
                    break;

                case LIB_MKB_KEY_3:
                    lib_buzzer_play_melody();
                    lib_lcd1602_write_str(0u, 1u, "Melody...      ");
                    LOG_I(TAG, "K3: melody queued");
                    break;

                case LIB_MKB_KEY_4:
                    lib_servo_set_target(SERVO_OPEN_DEG);
                    lib_buzzer_play_melody();
                    lib_lcd1602_write_str(0u, 1u, "Open + Melody! ");
                    LOG_I(TAG, "K4: servo open + melody");
                    break;

                default:
                    break;
            }
        }
    }
}

/* [] END OF FILE */
