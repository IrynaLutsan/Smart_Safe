/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "project.h"
#include "safe_types.h"
#include "event_queue.h"
#include "hardware.h"
#include "fsm.h"
#include "storage.h"
#include "lib_servo.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#define TAG "MAIN"
#include "log_dbg.h"


/********************************************************************************
 **********                             MAIN                          ***********
*********************************************************************************/

int main(void)
{
    hardware_init();
    storage_init();

    lib_servo_set_angle(SERVO_CLOSED_DEG);   /* 90° — neutral closed */

    SafeContext ctx = {
        .current_state      = &StateLocked,
        .backoff_multiplier = 1u,
    };
    ctx.current_state->on_enter(&ctx);

    LOG_I(TAG, "Safe booted — LOCKED");

    for (;;)
    {
        poll_hardware_and_push_events(&ctx);

        if (!event_queue_empty())
        {
            Event e;
            event_pop(&e);
            ctx.current_state->handle_event(&ctx, e.type, e.data);
        }
    }
}

/* [] END OF FILE */
