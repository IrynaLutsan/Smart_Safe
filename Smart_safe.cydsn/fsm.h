/**
 * @file fsm.h
 * @brief FSM engine: State interface, state declarations, and TRANSITION macro.
 *
 * Include hierarchy (no circular deps):
 *   safe_types.h  <-- fsm.h  <-- state_*.c / hardware.c
 *
 * safe_types.h forward-declares struct State; this header provides the full
 * definition, so state_*.c files only need to include fsm.h.
 */

#ifndef FSM_H
#define FSM_H

#include "safe_types.h"

/* ---- State interface ---- */

typedef struct State
{
    /** Called once when transitioning INTO this state. May be NULL. */
    void (*on_enter)(SafeContext* ctx);

    /** Called for every event while this state is active. Must not be NULL. */
    void (*handle_event)(SafeContext* ctx, EventType ev, void* data);

    /** Called once when transitioning OUT OF this state. May be NULL. */
    void (*on_exit)(SafeContext* ctx);
} State;

/* ---- State instances (defined in state_*.c) ---- */

extern State StateLocked;
extern State StateOpen;
extern State StateAlarm;
extern State State2FA;
extern State StateConfig;

/* ---- Transition macro ---- */

/**
 * @brief Execute a state transition.
 *
 * Calls on_exit of the current state (if set), updates the pointer,
 * then calls on_enter of the new state (if set).
 *
 * @param ctx   Pointer to SafeContext.
 * @param next  State variable (NOT a pointer) — e.g. TRANSITION(ctx, StateLocked).
 */
#define TRANSITION(ctx, next)                                       \
    do {                                                            \
        if ((ctx)->current_state->on_exit != NULL)                  \
            (ctx)->current_state->on_exit(ctx);                     \
        (ctx)->current_state = &(next);                             \
        if ((ctx)->current_state->on_enter != NULL)                 \
            (ctx)->current_state->on_enter(ctx);                    \
    } while (0)

#endif /* FSM_H */

/* [] END OF FILE */
