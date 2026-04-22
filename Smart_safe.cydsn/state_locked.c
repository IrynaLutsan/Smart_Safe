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
#include "lib_mkb.h"
#include <string.h>

#define LOG_LEVEL LOG_LEVEL_INFO
#include "log_dbg.h"
#define TAG "LOCKED"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
*********************************************************************************/

#define PIN_MAX_LEN  8u
#define MAX_FAILS    5u


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

static void show_mask(uint8_t len)
{
    char mask[17];
    uint8_t i;
    for (i = 0u; i < 16u; i++)
    {
        mask[i] = (i < len) ? '*' : ' ';
    }
    mask[16] = '\0';
    lib_lcd1602_write_str(0u, 1u, mask);
}

static void clear_input(SafeContext* ctx)
{
    ctx->input_len = 0u;
    memset(ctx->input_buffer, 0u, sizeof(ctx->input_buffer));
}


/********************************************************************************
 **********                     STATE IMPLEMENTATION                  ***********
*********************************************************************************/

static void on_enter(SafeContext* ctx)
{
    clear_input(ctx);
    LED_RED_Write(0u);
    LED_GREEN_Write(0u);
    lib_lcd1602_clear();
    lib_lcd1602_write_str(0u, 0u, "=== LOCKED ===");
    lib_lcd1602_write_str(0u, 1u, "                ");
    LOG_I(TAG, "entered");
}

static void handle_event(SafeContext* ctx, EventType ev, void* data)
{
    if (ev == EV_KEY_PRESS)
    {
        lib_mkb_key_t key = (lib_mkb_key_t)(uintptr_t)data;

        if (key == LIB_MKB_KEY_HASH)
        {
            /* Enter config mode */
            TRANSITION(ctx, StateConfig);
            return;
        }

        if (key == LIB_MKB_KEY_STAR)
        {
            /* Submit PIN */
            if (storage_verify_pin(ctx->input_buffer, ctx->input_len))
            {
                lib_buzzer_beep_success();
                TRANSITION(ctx, StateOpen);
            }
            else
            {
                ctx->failed_attempts++;
                lib_buzzer_beep_error();
                clear_input(ctx);
                show_mask(0u);
                LOG_I(TAG, "wrong PIN, fails=%u", (unsigned)ctx->failed_attempts);

                if (ctx->failed_attempts >= MAX_FAILS)
                {
                    TRANSITION(ctx, StateAlarm);
                }
            }
            return;
        }

        /* Digit key 0-9 */
        if ((uint8_t)key <= 9u && ctx->input_len < PIN_MAX_LEN)
        {
            ctx->input_buffer[ctx->input_len] = (uint8_t)key;
            ctx->input_len++;
            show_mask(ctx->input_len);
            lib_buzzer_beep_keypress();
        }
        return;
    }

    if (ev == EV_IMU_TRIP || ev == EV_MAG_TRIP || ev == EV_BARO_TRIP)
    {
        LOG_I(TAG, "tamper detected (ev=%d)", (int)ev);
        TRANSITION(ctx, State2FA);
        return;
    }
}

State StateLocked = { on_enter, handle_event, NULL };

/* [] END OF FILE */
