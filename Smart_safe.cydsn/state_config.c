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

#define PIN_MAX_LEN  8u
#define MAX_FAILS    3u

/* Sub-state used within AUTH_MENU to track what we're waiting for. */
typedef enum
{
    MENU_IDLE   = 0,   /* waiting for menu key (1/2/0) */
    MENU_NEWPIN,       /* collecting new PIN digits     */
    MENU_NEWTAG,       /* waiting for new RFID scan     */
} MenuSub;

static MenuSub g_menu_sub = MENU_IDLE;


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

static void clear_input(SafeContext* ctx)
{
    ctx->input_len = 0u;
    memset(ctx->input_buffer, 0u, sizeof(ctx->input_buffer));
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
        lib_lcd1602_write_str(0u, 0u, "1:PIN 2:TAG");
        lib_lcd1602_write_str(0u, 1u, "0:EXIT          ");
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
            g_menu_sub = MENU_NEWPIN;
            clear_input(ctx);
            lib_lcd1602_clear();
            lib_lcd1602_write_str(0u, 0u, "NEW PIN:");
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
        if (key == LIB_MKB_KEY_STAR && ctx->input_len > 0u)
        {
            storage_set_pin(ctx->input_buffer, ctx->input_len);
            lib_buzzer_beep_success();
            LOG_I(TAG, "new PIN saved (len=%u)", (unsigned)ctx->input_len);
            TRANSITION(ctx, StateLocked);
        }
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
    ctx->auth_step       = AUTH_PIN;
    ctx->failed_attempts = 0u;
    g_menu_sub           = MENU_IDLE;
    clear_input(ctx);

    lib_lcd1602_clear();
    lib_lcd1602_write_str(0u, 0u, "CONFIG > PIN:");
    lib_lcd1602_write_str(0u, 1u, "                ");

    LOG_I(TAG, "entered");
}

static void handle_event(SafeContext* ctx, EventType ev, void* data)
{
    if (ev == EV_KEY_PRESS)
    {
        lib_mkb_key_t key = (lib_mkb_key_t)(uintptr_t)data;

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
    (void)ctx;
    lib_servo_set_target(SERVO_CLOSED_DEG);
    g_menu_sub = MENU_IDLE;
    LOG_I(TAG, "exit");
}

State StateConfig = { on_enter, handle_event, on_exit };

/* [] END OF FILE */
