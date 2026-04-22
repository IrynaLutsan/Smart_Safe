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
#include "lib_seg_display.h"
#include "lib_acc_gyr.h"
#include "lib_magnetometer.h"
#include "lib_barometer.h"
#include "lib_adc.h"

#define TAG "HW"
#define LOG_LEVEL LOG_LEVEL_INFO
#include "log_dbg.h"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
*********************************************************************************/

#define SERVO_TICK_MS        10u
#define IMU_THRESHOLD_MG     1000
#define MAG_THRESHOLD_LSB    3000
#define BARO_THRESHOLD_PA    1500u

/* Light sensor (photoresistor/LDR on ADC_CH_EXT_2_5). */
#define LIGHT_SAMPLE_MS      100u
#define LIGHT_THRESHOLD      1500   /* tune on hardware — bright > ~1.2 V */
#define LIGHT_DEBOUNCE_HITS  3u

/* Baseline capture after sensor init. */
#define BASELINE_WARMUP_SAMPLES  3u
#define BASELINE_AVG_SAMPLES     8u
#define BASELINE_SAMPLE_DELAY_MS 10u


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

/* Tamper detectors are armed only when the safe should be physically idle.
 * In OPEN the user is handling the door; in ALARM/2FA we're already in a
 * bad-path sequence, so re-tripping adds noise without informational value. */
static uint8_t tamper_armed(const SafeContext* ctx)
{
    return (ctx->current_state == &StateLocked) ? 1u : 0u;
}


/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/

void hardware_init(void)
{
    dbg_log_init();

    CySysTickStart();
    CySysTickSetCallback(0u, systick_cb);

    I2C_Start();
    SPIM_Start();

    LED_RED_Write(0u);
    LED_GREEN_Write(0u);
    LED_BLUE_Write(0u);
    RELAY_Write(0u);

    lib_lcd1602_init();
    lib_lcd1602_clear();
    lib_seg_display_init();
    lib_seg_display_clear();
    lib_mkb_init();
    lib_servo_init();
    lib_buzzer_init();
    lib_acc_gyr_init();
    lib_magnetometer_init();
    lib_barometer_init();
    lib_adc_init();
    lib_rfid_init();

    /* Capture sensor baselines for tamper detection.
     * Discard a few warm-up samples (sensors often output zeros right after
     * power-up), then average a short window to smooth noise. */
    {
        uint8_t i;
        for (i = 0u; i < BASELINE_WARMUP_SAMPLES; i++)
        {
            CyDelay(BASELINE_SAMPLE_DELAY_MS);
            (void)lib_acc_gyr_get();
            (void)lib_magnetometer_get();
            (void)lib_barometer_get();
        }

        int32_t  acc_sum  = 0;
        int32_t  mag_sum  = 0;
        uint64_t baro_sum = 0u;
        for (i = 0u; i < BASELINE_AVG_SAMPLES; i++)
        {
            CyDelay(BASELINE_SAMPLE_DELAY_MS);
            lib_acc_gyr_data_t      a = lib_acc_gyr_get();
            lib_magnetometer_data_t m = lib_magnetometer_get();
            lib_barometer_data_t    b = lib_barometer_get();
            acc_sum  += accel_mag(&a);
            mag_sum  += mag_mag(&m);
            baro_sum += b.pressure;
        }
        g_accel_base_mag = acc_sum  / (int32_t)BASELINE_AVG_SAMPLES;
        g_mag_base_mag   = mag_sum  / (int32_t)BASELINE_AVG_SAMPLES;
        g_baro_base_pa   = (uint32_t)(baro_sum / BASELINE_AVG_SAMPLES);
    }

     /* Enable interrupts last — all drivers are up and their callbacks installed. */
    CyGlobalIntEnable;

    LOG_I(TAG, "hw init ok. accel_base=%ld mag_base=%ld baro_base=%lu",
          (long)g_accel_base_mag, (long)g_mag_base_mag, (unsigned long)g_baro_base_pa);
}

uint32_t sys_tick_ms(void)
{
    return g_ms;
}

uint8_t is_door_locked(void)
{
    /* REED_SW is active-low: 0 = magnet close (door shut), 1 = door open. */
    return (REED_SW_Read() == 0u) ? 1u : 0u;
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
        static uint8_t reed_last_locked = 0u;  /* 1 = door closed, 0 = door open */
        uint8_t reed_now_locked = is_door_locked();
        if (reed_now_locked && !reed_last_locked)
        {
            Event e;
            e.type = EV_REED_CLOSED;
            e.data = NULL;
            event_push(e);
        }
        reed_last_locked = reed_now_locked;
    }

    /* --- Alarm continuous buzzer tone --- */
    if (ctx->current_state == &StateAlarm)
    {
        lib_buzzer_start(1000u);
    }

    /* --- IMU tamper (only trip while physically idle and closed) --- */
    {
        lib_acc_gyr_data_t accel_data = lib_acc_gyr_get();
        if (accel_data.is_new.acc)
        {
            int32_t cur   = accel_mag(&accel_data);
            int32_t delta = cur - g_accel_base_mag;
            if (delta < 0) delta = -delta;
            if (delta > IMU_THRESHOLD_MG && tamper_armed(ctx))
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
            if (delta > MAG_THRESHOLD_LSB && tamper_armed(ctx))
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
        if (delta > BARO_THRESHOLD_PA && tamper_armed(ctx))
        {
            Event e;
            e.type = EV_BARO_TRIP;
            e.data = NULL;
            event_push(e);
        }
    }

    /* --- Light-inside-the-safe tamper (debounced photodiode sample) --- */
    {
        static uint32_t light_last_ms = 0u;
        static uint8_t  light_hits    = 0u;
        if (now - light_last_ms >= LIGHT_SAMPLE_MS)
        {
            light_last_ms = now;
            int16_t v = lib_adc_get(ADC_CH_EXT_2_5);
            if (v > LIGHT_THRESHOLD)
            {
                if (light_hits < LIGHT_DEBOUNCE_HITS)
                {
                    light_hits++;
                }
                if (light_hits >= LIGHT_DEBOUNCE_HITS && tamper_armed(ctx))
                {
                    Event e;
                    e.type = EV_LIGHT_TRIP;
                    e.data = NULL;
                    event_push(e);
                    light_hits = 0u; /* one event per sustained exposure */
                }
            }
            else
            {
                light_hits = 0u;
            }
        }
    }

    /* --- 7-segment countdown (shows remaining seconds for OPEN auto-lock and
     *     ALARM lockout). Refreshed once per second. --- */
    {
        static uint8_t  seg_was_showing = 0u;
        static uint32_t seg_last_ms     = 0u;

        uint8_t show_timer = (ctx->current_state == &StateOpen ||
                              ctx->current_state == &StateAlarm ||
                              ctx->current_state == &StateConfig) &&
                             ctx->timer_target_ms != 0u;

        if (show_timer)
        {
            seg_was_showing = 1u;
            if (now - seg_last_ms >= 1000u)
            {
                seg_last_ms = now;

                uint32_t remaining_s = (ctx->timer_target_ms > now)
                                       ? (ctx->timer_target_ms - now) / 1000u
                                       : 0u;

                uint8_t digits[LIB_SEG_DISPLAY_DIGITS_COUNT];
                uint8_t i;
                for (i = 0u; i < LIB_SEG_DISPLAY_DIGITS_COUNT; i++)
                {
                    digits[i] = SEG_DIGIT_BLANK;
                }

                if (remaining_s == 0u)
                {
                    digits[0] = 0u;
                }
                else
                {
                    uint8_t  pos = 0u;
                    uint32_t n   = remaining_s;
                    while (n > 0u && pos < LIB_SEG_DISPLAY_DIGITS_COUNT)
                    {
                        digits[pos++] = (uint8_t)(n % 10u);
                        n /= 10u;
                    }
                }
                lib_seg_display_update(digits);
            }
        }
        else if (seg_was_showing)
        {
            seg_was_showing = 0u;
            seg_last_ms     = 0u;
            lib_seg_display_clear();
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
