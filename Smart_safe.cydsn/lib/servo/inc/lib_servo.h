/**
 * @file lib_servo.h
 * @brief Servo motor control interface.
 */

#ifndef LIB_SERVO_H
#define LIB_SERVO_H

#include "lib_common_types.h"

#define SERVO_MIN_ANGLE_DEG  0u
#define SERVO_MID_ANGLE_DEG  90u
#define SERVO_MAX_ANGLE_DEG  180u

/** Logical positions used by the FSM. */
#define SERVO_CLOSED_DEG     0u   /**< RFID reader hidden.   */
#define SERVO_OPEN_DEG       90u  /**< RFID reader exposed.  */

/**
 * @brief Initialize the servo driver and snap to closed position (0 deg).
 */
void lib_servo_init(void);

/**
 * @brief Set the target angle — returns immediately (non-blocking).
 *
 * @param angle_deg Target angle 0..180. Clamped to SERVO_MAX_ANGLE_DEG.
 */
void lib_servo_set_target(uint16_t angle_deg);

/**
 * @brief Advance servo one degree toward the target. Call every 10 ms from poll loop.
 */
void lib_servo_tick(void);

/**
 * @brief Return the current servo angle in degrees.
 */
uint16_t lib_servo_get_angle(void);

/**
 * @brief Blocking wrapper — moves servo synchronously. Avoid in FSM context.
 *
 * @param angle_deg Target angle 0..180.
 */
void lib_servo_set_angle(uint16_t angle_deg);

#endif // LIB_SERVO_H

/* [] END OF FILE */
