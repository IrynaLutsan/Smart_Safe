/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "hardware.h"
#include "project.h"
#include "event_queue.h"
#include "fsm.h"
#include "storage.h"

#include "lib_servo.h"
#include "lib_buzzer.h"
#include "lib_mkb.h"
#include "lib_rfid.h"
#include "lib_lcd1602.h"
#include "lib_acc_gyr.h"
#include "lib_magnetometer.h"
#include "lib_barometer.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#include "log_dbg.h"
#define TAG "HW"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
*********************************************************************************/

#define SERVO_TICK_MS        10u
#define IMU_THRESHOLD_MG     200
#define MAG_THRESHOLD_LSB    50
#define BARO_THRESHOLD_PA    500u


/********************************************************************************
 **********                         PRIVATE VARIABLES                 ***********
*********************************************************************************/

static volatile uint32_t g_ms             = 0u;
static          int32_t  g_accel_base_mag = 0;
static          int32_t  g_mag_base_mag   = 0;
static          uint32_t g_baro_base_pa   = 0u;


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

static void systick_cb(void)
{
    g_ms++;
}

/* Manhattan-distance magnitude — avoids sqrt, good enough for tamper detection. */
static int32_t accel_mag(const lib_acc_gyr_data_t* d)
{
    int32_t x = d->acc.x < 0 ? -d->acc.x : d->acc.x;
    int32_t y = d->acc.y < 0 ? -d->acc.y : d->acc.y;
    int32_t z = d->acc.z < 0 ? -d->acc.z : d->acc.z;
    return x + y + z;
}

static int32_t mag_mag(const lib_magnetometer_data_t* d)
{
    int32_t x = d->mag.x < 0 ? -d->mag.x : d->mag.x;
    int32_t y = d->mag.y < 0 ? -d->mag.y : d->mag.y;
    int32_t z = d->mag.z < 0 ? -d->mag.z : d->mag.z;
    return x + y + z;
}


/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/

void hardware_init(void)
{
    CyGlobalIntEnable;

    dbg_log_init();

    CySysTickStart();
    CySysTickSetCallback(0u, systick_cb);

    I2C_Start();

    lib_lcd1602_init();
    lib_lcd1602_clear();
    lib_mkb_init();
    lib_servo_init();
    lib_buzzer_init();
    lib_acc_gyr_init();
    lib_magnetometer_init();
    lib_barometer_init();
    lib_rfid_init();

    storage_init();

    /* Capture sensor baselines for tamper detection. */
    lib_acc_gyr_data_t      accel_data = lib_acc_gyr_get();
    lib_magnetometer_data_t mag_data   = lib_magnetometer_get();
    lib_barometer_data_t    baro_data  = lib_barometer_get();

    g_accel_base_mag = accel_mag(&accel_data);
    g_mag_base_mag   = mag_mag(&mag_data);
    g_baro_base_pa   = baro_data.pressure;

    LOG_I(TAG, "hw init ok. accel_base=%ld mag_base=%ld baro_base=%lu",
          (long)g_accel_base_mag, (long)g_mag_base_mag, (unsigned long)g_baro_base_pa);
}

uint32_t sys_tick_ms(void)
{
    return g_ms;
}

void poll_hardware_and_push_events(SafeContext* ctx)
{
    uint32_t now = sys_tick_ms();

    /* --- Servo tick: advance one degree every 10 ms --- */
    static uint32_t servo_last_ms = 0u;
    if (now - servo_last_ms >= SERVO_TICK_MS)
    {
        lib_servo_tick();
        servo_last_ms = now;
    }

    /* --- Buzzer note-sequence state machine --- */
    lib_buzzer_tick(now);

    /* --- Keypad --- */
    {
        uint8_t keys[4][3];
        lib_mkb_result_t kb = lib_mkb_read(keys);
        if (kb.status == LIB_MKB_STATE_CHANGED && kb.key_state == LIB_MKB_KEY_PRESSED)
        {
            Event e;
            e.type = EV_KEY_PRESS;
            e.data = (void*)(uintptr_t)kb.key_code;
            event_push(e);
        }
    }

    /* --- RFID --- */
    {
        ret_code_t rc = lib_rfid_scan(ctx->rfid_uid);
        if (rc == RET_CODE_OK)
        {
            Event e;
            e.type = EV_RFID_SCANNED;
            e.data = ctx->rfid_uid;
            event_push(e);
        }
    }

    /* --- Reed switch (active-low, debounced via last-state tracking) --- */
    {
        static uint8_t reed_last = 1u;  /* 1 = open (not triggered), 0 = closed */
        uint8_t reed_now = REED_SW_Read();
        if (reed_now == 0u && reed_last != 0u)
        {
            Event e;
            e.type = EV_REED_CLOSED;
            e.data = NULL;
            event_push(e);
        }
        reed_last = reed_now;
    }

    /* --- Alarm continuous buzzer tone --- */
    if (ctx->current_state == &StateAlarm)
    {
        lib_buzzer_start(1000u);
    }

    /* --- IMU tamper (only trip when locked — prevents false alarm when open) --- */
    {
        lib_acc_gyr_data_t accel_data = lib_acc_gyr_get();
        if (accel_data.is_new.acc)
        {
            int32_t cur   = accel_mag(&accel_data);
            int32_t delta = cur - g_accel_base_mag;
            if (delta < 0) delta = -delta;
            if (delta > IMU_THRESHOLD_MG && ctx->current_state == &StateLocked)
            {
                Event e;
                e.type = EV_IMU_TRIP;
                e.data = NULL;
                event_push(e);
            }
        }
    }

    /* --- Magnetometer tamper --- */
    {
        lib_magnetometer_data_t mag_data = lib_magnetometer_get();
        if (mag_data.is_new)
        {
            int32_t cur   = mag_mag(&mag_data);
            int32_t delta = cur - g_mag_base_mag;
            if (delta < 0) delta = -delta;
            if (delta > MAG_THRESHOLD_LSB)
            {
                Event e;
                e.type = EV_MAG_TRIP;
                e.data = NULL;
                event_push(e);
            }
        }
    }

    /* --- Barometer tamper --- */
    {
        lib_barometer_data_t baro_data = lib_barometer_get();
        uint32_t delta = (baro_data.pressure > g_baro_base_pa)
                         ? (baro_data.pressure - g_baro_base_pa)
                         : (g_baro_base_pa   - baro_data.pressure);
        if (delta > BARO_THRESHOLD_PA)
        {
            Event e;
            e.type = EV_BARO_TRIP;
            e.data = NULL;
            event_push(e);
        }
    }

    /* --- Timeout --- */
    if (ctx->timer_target_ms != 0u && now >= ctx->timer_target_ms)
    {
        ctx->timer_target_ms = 0u;
        Event e;
        e.type = EV_TIMEOUT;
        e.data = NULL;
        event_push(e);
    }
}

/* [] END OF FILE */
