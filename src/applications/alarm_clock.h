#ifndef ALARM_CLOCK_H_
#define ALARM_CLOCK_H_

/* INCLUDES *****************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <ses_scheduler.h>

/* TYPES ********************************************************************/

/**
 * Forward declaration so state_t can reference fsm_t
 * and event_t before their full definitions
 */
typedef struct fsm_s    fsm_t;
typedef struct event_s  event_t;

// Event signals
enum signals {
    ENTRY,              // sent on state entry
    EXIT,               // sent on state exit
    ROTARY_PRESSED,
    ROTARY_CCW,     //rotary button pressed
    PUSH_PRESSED,       // push button pressed
    TICK_SECOND,        // one second elapsed
    ALARM_MATCH,        // current time == alarm
    ALARM_TIMEOUT       // 5 sec. auto alarm stop
};

// Return values for event handlers
enum return_values {
    RET_HANDLED,        // event handled, no transition
    RET_IGNORED,        // event ignored
    RET_TRANSITION      // state transition occured
};

typedef enum return_values fsm_return_status_t;

// State handler function pointer type
typedef fsm_return_status_t (*state_t)(fsm_t *, const event_t *);

// Event structure
struct event_s {
    uint8_t signal;     // identifies the type of event
};

// FSM structure
struct fsm_s {
    state_t state;          // current state handler
    bool    isAlarmEnabled; // alarm on/off flag
    time_t  timeSet;        // hours/minutes being set
    time_t  alarmTime;      // stored alarm time
};

/* FUNCTION PROTOTYPES *******************************************************/

/**
 * Initializes and starts the alarm clock fsm
 * Call once after scheduler_init() and before scheduler_run()
 */
void alarmClock_init(void);

#endif /* ALARM_CLOCK_H_*/