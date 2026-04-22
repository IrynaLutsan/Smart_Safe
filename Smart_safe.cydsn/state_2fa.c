/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "fsm.h"
#include "event_queue.h"
#include "storage.h"
#include "hardware.h"
#include "project.h"
#include "lib_lcd1602.h"
#include "lib_buzzer.h"
#include "lib_servo.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#include "log_dbg.h"
#define TAG "2FA"


/********************************************************************************
 **********                     STATE IMPLEMENTATION                  ***********
*********************************************************************************/

static void on_enter(SafeContext* ctx)
{
    lib_servo_set_target(SERVO_OPEN_DEG);   /* expose RFID reader */
    ctx->timer_target_ms = 0u;              /* no timeout — waits indefinitely */

    lib_lcd1602_clear();
    lib_lcd1602_write_str(0u, 0u, "SCAN RFID TAG");
    lib_lcd1602_write_str(0u, 1u, "Place on reader ");

    LOG_I(TAG, "entered — servo open, waiting for RFID");
}

static void on_exit(SafeContext* ctx)
{
    (void)ctx;
    lib_servo_set_target(SERVO_CLOSED_DEG);
    LOG_I(TAG, "exit — servo closing");
}

static void handle_event(SafeContext* ctx, EventType ev, void* data)
{
    (void)data;

    if (ev == EV_RFID_SCANNED)
    {
        if (storage_verify_rfid(ctx->rfid_uid))
        {
            LOG_I(TAG, "RFID valid — unlocking");
            lib_buzzer_beep_success();
            TRANSITION(ctx, StateLocked);
        }
        else
        {
            LOG_I(TAG, "RFID invalid — alarm");
            lib_lcd1602_clear();
            lib_lcd1602_write_str(0u, 0u, "INVALID TAG!");
            lib_lcd1602_write_str(0u, 1u, "-> ALARM        ");
            lib_buzzer_beep_error();
            TRANSITION(ctx, StateAlarm);
        }
    }
}

State State2FA = { on_enter, handle_event, on_exit };

/* [] END OF FILE */
