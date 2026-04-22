/**
 * @file lib_servo.h
 * @brief Servo motor control interface.
 */

#ifndef LIB_SERVO_H
#define LIB_SERVO_H

#include "lib_common_types.h"

/** @brief Minimum supported servo angle in degrees. */
#define SERVO_MIN_ANGLE_DEG 0u
/** @brief Middle servo angle in degrees. */
#define SERVO_MID_ANGLE_DEG 90u
/** @brief Maximum supported servo angle in degrees. */
#define SERVO_MAX_ANGLE_DEG 180u

/**
 * @brief Initialize the servo driver.
 *
 * Starts PWM output and sets servo to middle position.
 */
void lib_servo_init(void);

/**
 * @brief Set the servo position.
 *
 * @param angle_deg Target servo angle in degrees.
 * Values above SERVO_MAX_ANGLE_DEG are clamped.
 */
void lib_servo_set_angle(uint16_t angle_deg);

#endif // LIB_SERVO_H

/* [] END OF FILE */
