/**
 * @file safe_types.h
 * @brief Shared FSM types: EventType, AuthStep, Event, SafeContext.
 *
 * Include this header in every state file and in hardware.c.
 * The State struct is defined in fsm.h; forward-declared here so SafeContext
 * can hold a pointer to it without a circular dependency.
 */

#ifndef SAFE_TYPES_H
#define SAFE_TYPES_H

#include "lib_common_types.h"

/* ---- Forward declarations ---- */

struct State;       /* defined in fsm.h */
struct SafeContext; /* typedef'd below  */

/* ---- Event types ---- */

typedef enum
{
    EV_KEY_PRESS,    /**< Keypad key pressed. data = (void*)(uintptr_t)lib_mkb_key_t */
    EV_REED_CLOSED,  /**< Reed switch transitioned open -> closed. data = NULL */
    EV_IMU_TRIP,     /**< Accelerometer tamper threshold exceeded. data = NULL */
    EV_MAG_TRIP,     /**< Magnetometer anomaly detected. data = NULL */
    EV_BARO_TRIP,    /**< Barometric pressure spike detected. data = NULL */
    EV_TIMEOUT,      /**< ctx->timer_target_ms elapsed. data = NULL */
    EV_RFID_SCANNED, /**< Valid RFID read. data = ctx->rfid_uid (uint8_t[5]) */
} EventType;

/* ---- Event packet ---- */

typedef struct
{
    EventType type;
    void*     data;
} Event;

/* ---- Config sub-state ---- */

typedef enum
{
    AUTH_PIN,  /**< Waiting for admin PIN entry.        */
    AUTH_RFID, /**< Waiting for master RFID scan.       */
    AUTH_MENU, /**< Menu: 1=change PIN, 2=add tag, 0=exit */
} AuthStep;

/* ---- Shared FSM context ---- */

struct SafeContext
{
    struct State* current_state;      /**< Active state pointer.                  */
    uint8_t       failed_attempts;    /**< Wrong PIN / RFID attempts in state.    */
    uint8_t       backoff_multiplier; /**< Alarm timeout doubles each lockout.    */
    AuthStep      auth_step;          /**< Sub-state used inside StateConfig.     */
    uint8_t       input_buffer[16];   /**< Raw digit buffer for PIN entry.        */
    uint8_t       input_len;          /**< Number of digits currently in buffer.  */
    uint32_t      timer_target_ms;    /**< Absolute ms deadline; 0 = no timer.   */
    uint8_t       rfid_uid[5];        /**< Scratch buffer for last scanned UID.  */
};

typedef struct SafeContext SafeContext;

#endif /* SAFE_TYPES_H */

/* [] END OF FILE */
