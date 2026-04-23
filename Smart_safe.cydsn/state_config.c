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
#include "lib_mkb.h"
#include <string.h>

#define LOG_LEVEL LOG_LEVEL_INFO
#include "log_dbg.h"
#define TAG "CFG"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
*********************************************************************************/

#define PIN_MIN_LEN       4u
#define PIN_MAX_LEN       8u
#define MAX_FAILS         3u
#define CONFIG_IDLE_MS    30000u   /* auto-exit config after 30 s of inactivity */

/* Sub-state used within AUTH_MENU to track what we're waiting for. */
typedef enum
{
    MENU_IDLE   = 0,   /* waiting for menu key (1/2/3/0)    */
    MENU_NEWPIN,       /* collecting new PIN digits          */
    MENU_NEWPIN_CONFIRM,/* re-entering new PIN for confirm   */
    MENU_NEWTAG,       /* waiting for new RFID scan          */
    MENU_CLRTAG_CONFIRM,/* waiting for * to confirm erase    */
} MenuSub;

static MenuSub g_menu_sub = MENU_IDLE;

/* Scratch buffer for the first entry of a new PIN — compared against the
 * second entry in MENU_NEWPIN_CONFIRM before committing to NVM. */
static uint8_t g_new_pin[PIN_MAX_LEN];
static uint8_t g_new_pin_len = 0u;


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

static void clear_input(SafeContext* ctx)
{
    ctx->input_len = 0u;
    memset(ctx->input_buffer, 0u, sizeof(ctx->input_buffer));
}

static void refresh_idle_timer(SafeContext* ctx)
{
    ctx->timer_target_ms = sys_tick_ms() + CONFIG_IDLE_MS;
}

static void check_fail_guard(SafeContext* ctx)
{
    if (ctx->failed_attempts >= MAX_FAILS)
    {
        LOG_I(TAG, "too many failures — alarm");
        lib_servo_set_target(SERVO_CLOSED_DEG);
        TRANSITION(ctx, StateAlarm);
    }
}

static void handle_auth_pin(SafeContext* ctx, lib_mkb_key_t key)
{
    if ((uint8_t)key <= 9u && ctx->input_len < PIN_MAX_LEN)
    {
        ctx->input_buffer[ctx->input_len] = (uint8_t)key;
        ctx->input_len++;
        lib_buzzer_beep_keypress();
        return;
    }

    if (key == LIB_MKB_KEY_STAR)
    {
        if (storage_verify_pin(ctx->input_buffer, ctx->input_len))
        {
            LOG_I(TAG, "PIN ok — scan master tag");
            lib_buzzer_beep_success();
            clear_input(ctx);
            ctx->auth_step = AUTH_RFID;
            lib_servo_set_target(SERVO_OPEN_DEG);
            lib_lcd1602_clear();
            lib_lcd1602_write_str(0u, 0u, "SCAN MASTER TAG");
        }
        else
        {
            ctx->failed_attempts++;
            lib_buzzer_beep_error();
            clear_input(ctx);
            LOG_I(TAG, "wrong PIN in config, fails=%u", (unsigned)ctx->failed_attempts);
            check_fail_guard(ctx);
        }
    }
}

static void handle_auth_rfid(SafeContext* ctx)
{
    if (storage_verify_rfid(ctx->rfid_uid))
    {
        LOG_I(TAG, "master tag ok — menu");
        lib_buzzer_beep_success();
        lib_servo_set_target(SERVO_CLOSED_DEG);
        ctx->auth_step = AUTH_MENU;
        g_menu_sub     = MENU_IDLE;
        lib_lcd1602_clear();
        lib_lcd1602_write_str(0u, 0u, "1:PIN 2:+TAG");
        lib_lcd1602_write_str(0u, 1u, "3:CLR 0:EXIT    ");
    }
    else
    {
        ctx->failed_attempts++;
        lib_buzzer_beep_error();
        LOG_I(TAG, "wrong tag in config, fails=%u", (unsigned)ctx->failed_attempts);
        check_fail_guard(ctx);
    }
}

static void handle_menu_key(SafeContext* ctx, lib_mkb_key_t key)
{
    if (g_menu_sub == MENU_IDLE)
    {
        if (key == LIB_MKB_KEY_0)
        {
            LOG_I(TAG, "exit config");
            TRANSITION(ctx, StateLocked);
            return;
        }
        if (key == LIB_MKB_KEY_1)
        {
            g_menu_sub     = MENU_NEWPIN;
            g_new_pin_len  = 0u;
            memset(g_new_pin, 0u, sizeof(g_new_pin));
            clear_input(ctx);
            lib_lcd1602_clear();
            lib_lcd1602_write_str(0u, 0u, "NEW PIN (>=4):");
            lib_lcd1602_write_str(0u, 1u, "then press *    ");
            return;
        }
        if (key == LIB_MKB_KEY_2)
        {
            g_menu_sub = MENU_NEWTAG;
            lib_servo_set_target(SERVO_OPEN_DEG);
            lib_lcd1602_clear();
            lib_lcd1602_write_str(0u, 0u, "SCAN NEW TAG");
            lib_lcd1602_write_str(0u, 1u, "scan RFID now   ");
            return;
        }
        if (key == LIB_MKB_KEY_3)
        {
            g_menu_sub = MENU_CLRTAG_CONFIRM;
            lib_lcd1602_clear();
            lib_lcd1602_write_str(0u, 0u, "CLEAR ALL TAGS?");
            lib_lcd1602_write_str(0u, 1u, "*=yes  0=cancel ");
            return;
        }
        return;
    }

    if (g_menu_sub == MENU_NEWPIN)
    {
        if ((uint8_t)key <= 9u && ctx->input_len < PIN_MAX_LEN)
        {
            ctx->input_buffer[ctx->input_len] = (uint8_t)key;
            ctx->input_len++;
            lib_buzzer_beep_keypress();
            return;
        }
        if (key == LIB_MKB_KEY_STAR)
        {
            if (ctx->input_len < PIN_MIN_LEN)
            {
                lib_buzzer_beep_error();
                clear_input(ctx);
                lib_lcd1602_write_str(0u, 0u, "NEED >=4 DIGITS");
                lib_lcd1602_write_str(0u, 1u, "try again...    ");
                return;
            }
            /* Stash the first entry and ask for confirmation. */
            memcpy(g_new_pin, ctx->input_buffer, ctx->input_len);
            g_new_pin_len = ctx->input_len;
            clear_input(ctx);
            g_menu_sub = MENU_NEWPIN_CONFIRM;
            lib_buzzer_beep_keypress();
            lib_lcd1602_clear();
            lib_lcd1602_write_str(0u, 0u, "RE-ENTER PIN:");
            lib_lcd1602_write_str(0u, 1u, "then press *    ");
        }
        return;
    }

    if (g_menu_sub == MENU_NEWPIN_CONFIRM)
    {
        if ((uint8_t)key <= 9u && ctx->input_len < PIN_MAX_LEN)
        {
            ctx->input_buffer[ctx->input_len] = (uint8_t)key;
            ctx->input_len++;
            lib_buzzer_beep_keypress();
            return;
        }
        if (key == LIB_MKB_KEY_STAR)
        {
            if (ctx->input_len == g_new_pin_len &&
                memcmp(ctx->input_buffer, g_new_pin, g_new_pin_len) == 0)
            {
                storage_set_pin(g_new_pin, g_new_pin_len);
                lib_buzzer_beep_success();
                LOG_I(TAG, "new PIN saved (len=%u)", (unsigned)g_new_pin_len);
                /* Wipe scratch before leaving. */
                memset(g_new_pin, 0u, sizeof(g_new_pin));
                g_new_pin_len = 0u;
                TRANSITION(ctx, StateLocked);
            }
            else
            {
                /* Mismatch is a UX failure, not an attack — don't bump the
                 * 3-strike counter. Restart the PIN-change flow. */
                lib_buzzer_beep_error();
                memset(g_new_pin, 0u, sizeof(g_new_pin));
                g_new_pin_len = 0u;
                clear_input(ctx);
                g_menu_sub = MENU_IDLE;
                lib_lcd1602_clear();
                lib_lcd1602_write_str(0u, 0u, "PIN MISMATCH");
                lib_lcd1602_write_str(0u, 1u, "1:PIN 2:+TAG    ");
                LOG_I(TAG, "PIN confirm mismatch — flow restarted");
            }
        }
        return;
    }

    if (g_menu_sub == MENU_CLRTAG_CONFIRM)
    {
        if (key == LIB_MKB_KEY_STAR)
        {
            storage_clear_rfid();
            lib_buzzer_beep_success();
            LOG_I(TAG, "RFID list cleared from config");
            TRANSITION(ctx, StateLocked);
            return;
        }
        if (key == LIB_MKB_KEY_0)
        {
            g_menu_sub = MENU_IDLE;
            lib_lcd1602_clear();
            lib_lcd1602_write_str(0u, 0u, "1:PIN 2:+TAG");
            lib_lcd1602_write_str(0u, 1u, "3:CLR 0:EXIT    ");
        }
        return;
    }
}

static void handle_menu_rfid(SafeContext* ctx)
{
    if (g_menu_sub == MENU_NEWTAG)
    {
        storage_add_rfid(ctx->rfid_uid);
        lib_servo_set_target(SERVO_CLOSED_DEG);
        lib_buzzer_beep_success();
        LOG_I(TAG, "new RFID tag saved");
        TRANSITION(ctx, StateLocked);
    }
}


/********************************************************************************
 **********                     STATE IMPLEMENTATION                  ***********
*********************************************************************************/

static void on_enter(SafeContext* ctx)
{
    /* NOTE: we intentionally do NOT reset ctx->failed_attempts here.
     * The FSM rule is: the wrong-PIN/RFID counter is cleared only on a
     * legitimate successful entry into StateOpen. Otherwise an attacker
     * could reset the counter by mashing '#' into LOCKED. */
    ctx->auth_step = AUTH_PIN;
    g_menu_sub     = MENU_IDLE;
    g_new_pin_len  = 0u;
    memset(g_new_pin, 0u, sizeof(g_new_pin));
    clear_input(ctx);
    refresh_idle_timer(ctx);

    lib_lcd1602_clear();
    lib_lcd1602_write_str(0u, 0u, "CONFIG > PIN:");
    lib_lcd1602_write_str(0u, 1u, "*=ok  0=cancel  ");

    LOG_I(TAG, "entered (fails carried=%u)", (unsigned)ctx->failed_attempts);
}

static void handle_event(SafeContext* ctx, EventType ev, void* data)
{
    /* Any user activity refreshes the idle timer. */
    if (ev == EV_KEY_PRESS || ev == EV_RFID_SCANNED)
    {
        refresh_idle_timer(ctx);
    }

    if (ev == EV_TIMEOUT)
    {
        LOG_I(TAG, "idle timeout — back to LOCKED");
        TRANSITION(ctx, StateLocked);
        return;
    }

    if (ev == EV_KEY_PRESS)
    {
        lib_mkb_key_t key = (lib_mkb_key_t)(uintptr_t)data;

        /* Global cancel from any step: '0' in AUTH_PIN aborts config. */
        if (ctx->auth_step == AUTH_PIN && key == LIB_MKB_KEY_0 &&
            ctx->input_len == 0u)
        {
            LOG_I(TAG, "cancel from PIN step");
            TRANSITION(ctx, StateLocked);
            return;
        }

        switch (ctx->auth_step)
        {
            case AUTH_PIN:
                handle_auth_pin(ctx, key);
                break;

            case AUTH_MENU:
                handle_menu_key(ctx, key);
                break;

            default:
                break;
        }
        return;
    }

    if (ev == EV_RFID_SCANNED)
    {
        switch (ctx->auth_step)
        {
            case AUTH_RFID:
                handle_auth_rfid(ctx);
                break;

            case AUTH_MENU:
                handle_menu_rfid(ctx);
                break;

            default:
                break;
        }
    }
}

static void on_exit(SafeContext* ctx)
{
    lib_servo_set_target(SERVO_CLOSED_DEG);
    g_menu_sub = MENU_IDLE;
    memset(g_new_pin, 0u, sizeof(g_new_pin));
    g_new_pin_len        = 0u;
    ctx->timer_target_ms = 0u;
    LOG_I(TAG, "exit");
}

State StateConfig = { on_enter, handle_event, on_exit };

/* [] END OF FILE */
