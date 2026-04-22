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

/** Servo pulse width limits in microseconds. */
#define SERVO_MIN_PULSE_US 500u
#define SERVO_MAX_PULSE_US 2400u
#define SERVO_STEP_DELAY_MS 10u

static uint16_t g_servo_current_angle_deg = SERVO_MID_ANGLE_DEG + 1;


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief Convert a servo angle to a PWM compare value.
 * @param angle_deg Angle in degrees, clamped to 0..180.
 * @return PWM compare value for the generated PWM_SERVO component.
 */
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

/**
 * @brief Initialize servo PWM and set default middle angle.
 *
 * Must be called once before lib_servo_set_angle().
 */
void lib_servo_init(void)
{
    LOG_I(TAG, "Initializing servo motor...");
    PWM2_SERVO_Init();
    LOG_D(TAG, "PWM2_SERVO initialized");
    PWM2_SERVO_Enable();
    LOG_D(TAG, "PWM2_SERVO enabled");
	PWM2_SERVO_Start();
	LOG_D(TAG, "PWM2_SERVO started");
	lib_servo_set_angle(SERVO_MID_ANGLE_DEG);
	LOG_I(TAG, "Servo initialization complete, set to 90 degrees");
}

/**
 * @brief Move servo to requested angle with smooth 1-degree steps.
 *
 * @param angle_deg Desired angle in degrees (0..180). Values above 180 are clamped.
 */
void lib_servo_set_angle(uint16_t angle_deg)
{
	if (angle_deg > 180u)
	{
		angle_deg = 180u;
	}

	while (g_servo_current_angle_deg != angle_deg)
	{
		if (g_servo_current_angle_deg < angle_deg)
		{
			g_servo_current_angle_deg++;
		}
		else
		{
			g_servo_current_angle_deg--;
		}

		PWM2_SERVO_WriteCompare(lib_servo_angle_to_compare(g_servo_current_angle_deg));
		CyDelay(SERVO_STEP_DELAY_MS);
	}

	LOG_D(TAG, "Servo angle set to %d degrees", angle_deg);
}

/* [] END OF FILE */
