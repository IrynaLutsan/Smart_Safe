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
#include <stdio.h>   /* snprintf */

#define LOG_LEVEL LOG_LEVEL_INFO
#include "log_dbg.h"
#define TAG "2FA"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
*********************************************************************************/

/* User UX: allow a few mistaken scans within a short window before re-arming
 * the alarm. A single wrong scan is more often "grabbed the wrong card" than
 * an attack. */
#define MAX_BAD_SCANS    3u
#define BAD_SCAN_WIN_MS  30000u


/********************************************************************************
 **********                     STATE IMPLEMENTATION                  ***********
*********************************************************************************/

static void on_enter(SafeContext* ctx)
{
    lib_servo_set_target(SERVO_OPEN_DEG);   /* expose RFID reader */
    ctx->timer_target_ms        = 0u;       /* no timeout — waits indefinitely */
    ctx->bad_scan_count         = 0u;
    ctx->bad_scan_window_start  = 0u;

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
            return;
        }

        /* Wrong tag — slide the retry window, bump the count. */
        uint32_t now = sys_tick_ms();
        if (ctx->bad_scan_count == 0u ||
            now - ctx->bad_scan_window_start > BAD_SCAN_WIN_MS)
        {
            ctx->bad_scan_count        = 1u;
            ctx->bad_scan_window_start = now;
        }
        else
        {
            ctx->bad_scan_count++;
        }

        lib_buzzer_beep_error();
        LOG_I(TAG, "RFID invalid (%u/%u)",
              (unsigned)ctx->bad_scan_count, (unsigned)MAX_BAD_SCANS);

        if (ctx->bad_scan_count >= MAX_BAD_SCANS)
        {
            lib_lcd1602_clear();
            lib_lcd1602_write_str(0u, 0u, "TOO MANY BAD TAGS");
            lib_lcd1602_write_str(0u, 1u, "-> ALARM        ");
            TRANSITION(ctx, StateAlarm);
        }
        else
        {
            /* Stay in 2FA, tell the user how many tries they have left. */
            char line[17];
            snprintf(line, sizeof(line), "INVALID %u/%u      ",
                     (unsigned)ctx->bad_scan_count, (unsigned)MAX_BAD_SCANS);
            line[16] = '\0';
            lib_lcd1602_write_str(0u, 0u, "SCAN RFID TAG   ");
            lib_lcd1602_write_str(0u, 1u, line);
        }
    }
}

State State2FA = { on_enter, handle_event, on_exit };

/* [] END OF FILE */
