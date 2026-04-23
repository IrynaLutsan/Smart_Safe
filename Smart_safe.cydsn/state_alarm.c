/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "fsm.h"
#include "event_queue.h"
#include "hardware.h"
#include "project.h"
#include "lib_lcd1602.h"
#include "lib_buzzer.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#include "log_dbg.h"
#define TAG "ALARM"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
*********************************************************************************/

#define ALARM_BASE_MS  5000u   /* base timeout before 2FA prompt */
#define BACKOFF_MAX    8u      /* caps the shift to avoid overflow */


/********************************************************************************
 **********                     STATE IMPLEMENTATION                  ***********
*********************************************************************************/

static void on_enter(SafeContext* ctx)
{
    LED_RED_Write(1u);

    /* Exponential backoff: 5 s × 2^backoff_multiplier */
    uint8_t  shift = (ctx->backoff_multiplier < BACKOFF_MAX)
                     ? ctx->backoff_multiplier : BACKOFF_MAX;
    uint32_t delay_ms = ALARM_BASE_MS << shift;
    ctx->timer_target_ms = sys_tick_ms() + delay_ms;
    ctx->backoff_multiplier++;

    lib_lcd1602_clear();
    lib_lcd1602_write_str(0u, 0u, "!!! ALARM !!!");
    lib_lcd1602_write_str(0u, 1u, "Scan RFID tag   ");

    /* Continuous buzzer tone is driven by poll_hardware_and_push_events()
     * while current_state == &StateAlarm. */

    LOG_I(TAG, "entered — timeout in %lu ms (mult=%u)",
          (unsigned long)delay_ms, (unsigned)ctx->backoff_multiplier);
}

static void on_exit(SafeContext* ctx)
{
    (void)ctx;
    LED_RED_Write(0u);
    lib_buzzer_stop();
    LOG_I(TAG, "exit");
}

static void handle_event(SafeContext* ctx, EventType ev, void* data)
{
    (void)data;

    /* Block both keypad and RFID input during the alarm — the user's spec
     * grants RFID authority only in State2FA. Tamper events while already
     * alarming are redundant and only pollute logs. */
    if (ev != EV_TIMEOUT)
    {
        return;
    }

    if (ev == EV_TIMEOUT)
    {
        LOG_I(TAG, "timeout — transition to 2FA");
        TRANSITION(ctx, State2FA);
    }
}

State StateAlarm = { on_enter, handle_event, on_exit };

/* [] END OF FILE */
