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
#define TAG "OPEN"


/********************************************************************************
 **********                     STATE IMPLEMENTATION                  ***********
*********************************************************************************/

static void on_enter(SafeContext* ctx)
{
    ctx->failed_attempts    = 0u;
    ctx->backoff_multiplier = 1u;

    RELAY_Write(1u);
    LED_GREEN_Write(1u);

    lib_lcd1602_clear();
    lib_lcd1602_write_str(0u, 0u, "=== OPEN ===");
    lib_lcd1602_write_str(0u, 1u, "Close to lock   ");

    LOG_I(TAG, "entered — relay on");
}

static void on_exit(SafeContext* ctx)
{
    (void)ctx;
    RELAY_Write(0u);
    LED_GREEN_Write(0u);
    LOG_I(TAG, "exit — relay off");
}

static void handle_event(SafeContext* ctx, EventType ev, void* data)
{
    (void)data;
    if (ev == EV_REED_CLOSED)
    {
        LOG_I(TAG, "reed closed — locking");
        TRANSITION(ctx, StateLocked);
    }
}

State StateOpen = { on_enter, handle_event, on_exit };

/* [] END OF FILE */
