/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "fsm.h"
#include "event_queue.h"
#include "hardware.h"
#include "project.h"
#include "lib_lcd1602.h"
#include "lib_buzzer.h"
#include "lib_servo.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#include "log_dbg.h"
#define TAG "OPEN"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
*********************************************************************************/

#define OPEN_TIMEOUT_MS  60000u   /* auto-lock after 60 s of inactivity */
#define GRACE_PERIOD_MS   3000u   /* ignore reed for first 3 s (solenoid settle) */


/********************************************************************************
 **********                         PRIVATE VARIABLES                 ***********
*********************************************************************************/

static uint32_t g_open_entry_ms = 0u;


/********************************************************************************
 **********                     STATE IMPLEMENTATION                  ***********
*********************************************************************************/

static void on_enter(SafeContext* ctx)
{
    ctx->failed_attempts    = 0u;
    ctx->backoff_multiplier = 1u;

    g_open_entry_ms      = sys_tick_ms();
    ctx->timer_target_ms = g_open_entry_ms + OPEN_TIMEOUT_MS;

    RELAY_Write(1u);          /* solenoid fires — door unlocks */
    LED_GREEN_Write(1u);
    lib_servo_set_target(SERVO_DOOR_OPEN_DEG);   /* swing door open (180°) */

    lib_lcd1602_clear();
    lib_lcd1602_write_str(0u, 0u, "** OPEN **");
    lib_lcd1602_write_str(0u, 1u, "Close door->LOCK");

    LOG_I(TAG, "entered — relay on, servo 180");
}

static void on_exit(SafeContext* ctx)
{
    (void)ctx;
    lib_servo_set_target(SERVO_CLOSED_DEG);   /* return to 90° neutral */
    RELAY_Write(0u);
    LED_GREEN_Write(0u);
    LOG_I(TAG, "exit — relay off, servo closing");
}

static void handle_event(SafeContext* ctx, EventType ev, void* data)
{
    (void)data;

    if (ev == EV_REED_CLOSED)
    {
        /* Ignore reed during grace period — solenoid fires and reed bounces */
        if (sys_tick_ms() - g_open_entry_ms >= GRACE_PERIOD_MS)
        {
            LOG_I(TAG, "reed closed — locking");
            TRANSITION(ctx, StateLocked);
        }
        return;
    }

    if (ev == EV_TIMEOUT)
    {
        LOG_I(TAG, "open timeout — auto-locking");
        TRANSITION(ctx, StateLocked);
    }
}

State StateOpen = { on_enter, handle_event, on_exit };

/* [] END OF FILE */
