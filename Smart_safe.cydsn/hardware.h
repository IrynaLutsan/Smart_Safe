/**
 * @file hardware.h
 * @brief Hardware initialisation, millisecond clock, and sensor poll.
 */

#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include "safe_types.h"

/**
 * @brief Initialise all peripherals and capture sensor baselines.
 *
 * Call once from main() before entering the FSM loop.
 * Enables global interrupts, starts SysTick, initialises every lib driver,
 * and records baseline accelerometer magnitude and barometric pressure.
 */
void hardware_init(void);

/**
 * @brief Return the current millisecond counter value.
 *
 * Driven by the SysTick callback installed in hardware_init().
 */
uint32_t sys_tick_ms(void);

/**
 * @brief Poll all sensors and push any resulting events onto the event queue.
 *
 * Must be called from the main loop on every iteration.
 * Also advances the servo and buzzer non-blocking state machines.
 *
 * @param ctx  Pointer to the FSM context (read for alarm-state check, written
 *             for rfid_uid scratch and timer reset).
 */
void poll_hardware_and_push_events(SafeContext* ctx);

#endif /* HARDWARE_H */

/* [] END OF FILE */
