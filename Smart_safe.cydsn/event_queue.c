/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "event_queue.h"
#include "project.h"    /* CyEnterCriticalSection / CyExitCriticalSection */


/********************************************************************************
 **********                         PRIVATE VARIABLES                 ***********
*********************************************************************************/

static Event   q[EVENT_QUEUE_SIZE];
static uint8_t q_head  = 0u;  /* index of next slot to write */
static uint8_t q_tail  = 0u;  /* index of next slot to read  */
static uint8_t q_count = 0u;  /* number of events stored     */


/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/

void event_push(Event e)
{
    uint8_t intr = CyEnterCriticalSection();

    if (q_count < EVENT_QUEUE_SIZE)
    {
        q[q_head] = e;
        q_head = (uint8_t)((q_head + 1u) % EVENT_QUEUE_SIZE);
        q_count++;
    }
    /* else: queue full — event dropped */

    CyExitCriticalSection(intr);
}

uint8_t event_pop(Event* out)
{
    if (q_count == 0u)
    {
        return 0u;
    }

    *out   = q[q_tail];
    q_tail = (uint8_t)((q_tail + 1u) % EVENT_QUEUE_SIZE);
    q_count--;

    return 1u;
}

uint8_t event_queue_empty(void)
{
    return (q_count == 0u) ? 1u : 0u;
}

/* [] END OF FILE */
