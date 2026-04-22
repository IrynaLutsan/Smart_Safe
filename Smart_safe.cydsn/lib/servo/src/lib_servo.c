/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "lib_servo.h"
#include "project.h"
#include "PWM_SERVO.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#include "log_dbg.h"
#define TAG "SERVO"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
*********************************************************************************/

#define SERVO_MIN_PULSE_US  500u
#define SERVO_MAX_PULSE_US  2400u
#define SERVO_STEP_DELAY_MS 10u  /* used only by blocking lib_servo_set_angle() */

static uint16_t g_current_deg = SERVO_CLOSED_DEG;
static uint16_t g_target_deg  = SERVO_CLOSED_DEG;


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

static uint16_t lib_servo_angle_to_compare(uint16_t angle_deg)
{
    if (angle_deg > 180u)
    {
        angle_deg = 180u;
    }

    uint32_t pulse_us = SERVO_MIN_PULSE_US +
        ((uint32_t)angle_deg * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) + 90u) / 180u;

    return (uint16_t)pulse_us;
}


/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/

void lib_servo_init(void)
{
    LOG_I(TAG, "Servo init -> closed (0 deg)");
    PWM2_SERVO_Init();
    PWM2_SERVO_Enable();
    PWM2_SERVO_Start();

    g_current_deg = SERVO_CLOSED_DEG;
    g_target_deg  = SERVO_CLOSED_DEG;
    PWM2_SERVO_WriteCompare(lib_servo_angle_to_compare(SERVO_CLOSED_DEG));
}

void lib_servo_set_target(uint16_t angle_deg)
{
    if (angle_deg > SERVO_MAX_ANGLE_DEG)
    {
        angle_deg = SERVO_MAX_ANGLE_DEG;
    }
    g_target_deg = angle_deg;
    LOG_D(TAG, "Target -> %d deg", angle_deg);
}

void lib_servo_tick(void)
{
    if (g_current_deg == g_target_deg)
    {
        return;
    }

    if (g_current_deg < g_target_deg)
    {
        g_current_deg++;
    }
    else
    {
        g_current_deg--;
    }

    PWM2_SERVO_WriteCompare(lib_servo_angle_to_compare(g_current_deg));
}

uint16_t lib_servo_get_angle(void)
{
    return g_current_deg;
}

/* Blocking wrapper — kept for compatibility. Not used in FSM context. */
void lib_servo_set_angle(uint16_t angle_deg)
{
    lib_servo_set_target(angle_deg);
    while (g_current_deg != g_target_deg)
    {
        lib_servo_tick();
        CyDelay(SERVO_STEP_DELAY_MS);
    }
    LOG_D(TAG, "Blocking move complete: %d deg", angle_deg);
}

/* [] END OF FILE */
