/**
 * @file event_queue.h
 * @brief Statically allocated, interrupt-safe FIFO event queue.
 *
 * Capacity: EVENT_QUEUE_SIZE slots (compile-time constant).
 * event_push() is safe to call from ISR context.
 * event_pop() / event_queue_empty() must be called from main-loop context only.
 */

#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include "safe_types.h"

#define EVENT_QUEUE_SIZE 16u

/**
 * @brief Push an event onto the queue.
 *
 * Disables global interrupts briefly for atomicity.
 * Drops the event silently if the queue is full.
 *
 * @param e Event to push.
 */
void event_push(Event e);

/**
 * @brief Pop the oldest event from the queue.
 *
 * @param[out] out Destination for the popped event.
 * @return 1 if an event was available, 0 if the queue was empty.
 */
uint8_t event_pop(Event* out);

/**
 * @brief Return 1 if the queue contains no events, 0 otherwise.
 */
uint8_t event_queue_empty(void);

#endif /* EVENT_QUEUE_H */

/* [] END OF FILE */
