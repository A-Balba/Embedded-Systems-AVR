/* INCLUDES *****************************************************************/

#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include "alarm_clock.h"
#include "ses_scheduler.h"
#include "ses_button.h"
#include "ses_led.h"
#include "ses_display.h"

/* DEFINES & MACROS **********************************************************/

#define DEBOUNCE_PERIOD         5       // ms - button polling interval
#define TICK_SECOND_PERIOD      1000    // ms - 1 sec scheduler task
#define ALARM_CHECK_PERIOD      1000    // ms - alarm match check
#define ALARM_DURATION_MS       5000    // ms - auto stop after 5 sec
#define RED_LED_FLASH_PERIOD    125     // ms - 4 Hz = 250 ms period (toggle every 125 ms)

#define MS_PER_HOUR             (60UL * 60UL * 1000UL)
#define MS_PER_MIN              (60UL * 1000UL)
#define HOURS_PER_DAY           24
#define MINS_PER_HOUR           60

/* FORWARD DECLARATIONS - state handler functions ****************************/

static fsm_return_status_t state_uninitHour(fsm_t * fsm, const event_t * e);
static fsm_return_status_t state_uninitMin (fsm_t * fsm, const event_t * e);
static fsm_return_status_t state_normalOp  (fsm_t * fsm, const event_t * e);
static fsm_return_status_t state_setAlarmHour(fsm_t * fsm, const event_t * e);
static fsm_return_status_t state_setAlarmMin (fsm_t * fsm, const event_t * e);
static fsm_return_status_t state_alarmRinging(fsm_t * fsm, const event_t * e);

/* PRIVATE VARIABLES *********************************************************/

static fsm_t theFsm;

/* Task descriptors - static so they never go out of scope */
static task_descriptor_t debounceTask;
static task_descriptor_t tickSecondTask;
static task_descriptor_t alarmCheckTask;
static task_descriptor_t redFlashTask;
static task_descriptor_t alarmTimeoutTask;   // one-shot

/* PRIVATE FUNCTIONS - FSM infrastucture *************************************/

static inline void fsm_init(fsm_t * fsm, state_t init) {
    const event_t entryEvent = { .signal = ENTRY };
    fsm->state = init;
    fsm->state(fsm, &entryEvent);
}

static inline fsm_dispatch(fsm_t * fsm, const event_t * event) {
    const event_t entryEvent = { .signal = ENTRY };
    const event_t exitEvent = { .signal = EXIT };
    state_t s = fsm->state;
    fsm_return_status_t r = fsm->state(fsm, event);
    if (r == RET_TRANSITION) {
        s(fsm, &exitEvent);
        fsm->state(fsm, &entryEvent);
    }
}

/* PRIVATE FUNCTIONS - display helpers ***************************************/

static void display_time(time_t t) {
    struct tm tm;
    scheduler_toTm(t, &tm);
    display_clear();
    display_setCursor(0, 0);
    fprintf(displayout, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    display_update();
}

static void display_hhmm(time_t t, const char * line2) {
    struct tm tm;
    scheduler_toTm(t, &tm);
    display_clear();
    display_setCursor(0, 0);
    fprintf(displayout, "%02d:%02d", tm.tm_hour, tm.tm_min);
    display_setCursor(0, 1);
    fprintf(displayout, "%s", line2);
    display_update();
}

/* PRIVATE FUNCTIONS - scheduler task callbacks ******************************/

static void task_debounce(void * param) {
    (void)param;
    button_checkState();
}

static void task_tickSecond(void * param) {
    (void)param;
    const event_t e = { .signal = TICK_SECOND };
    fsm_dispatch(&theFsm, &e);
}

static void task_alarmCheck(void * param) {
    (void)param;
    if (!theFsm.isAlarmEnabled) {
        return;
    }
    // Compare current time to alarm time - match within same sec
    struct tm now, alarm;
    scheduler_toTm(scheduler_getTime(), &now);
    scheduler_toTm(theFsm.alarmTime, &alarm);

    if (now.tm_hour == alarm.tm_hour && now.tm_min == alarm.tm_min && now.tm_sec == 0) {
        const event_t e = { .signal = ALARM_MATCH };
        fsm_dispatch(&theFsm, &e);
    }
}

static void task_redFlash(void * param) {
    (void)param;
    // led_redToggle();
    if (theFsm.state == state_alarmRinging) {
        led_redToggle();
    } else {
        led_redOff();
    }
}

static void task_alarmTimeout(void * param) {
    (void)param;
    const event_t e = { .signal = ALARM_TIMEOUT };
    fsm_dispatch(&theFsm, &e);
    // one-shot, scheduler removes it automatically
}

/* PRIVATE FUNCTIONS - button callbacks **************************************/

static void onRotaryPressed(void) {
    const event_t e = { .signal = ROTARY_PRESSED };
    fsm_dispatch(&theFsm, &e);
}

static void onPushPressed(void) {
    const event_t e = { .signal = PUSH_PRESSED };
    fsm_dispatch(&theFsm, &e);
}

static void onRotaryCW(void) {
    const event_t e = { .signal = ROTARY_PRESSED };
    fsm_dispatch(&theFsm, &e);
}

static void onRotaryCCW(void) {
    const event_t e = { .signal = ROTARY_CCW };
    fsm_dispatch(&theFsm, &e);
}

/* PRIVATE FUNCTIONS - state handlers ***************************************/

static fsm_return_status_t state_uninitHour(fsm_t * fsm, const event_t * e) {
    switch (e->signal) {
    case ENTRY:
        fsm->timeSet = 0;
        display_hhmm(fsm->timeSet, "Set hour");
        return RET_HANDLED;
    
    case ROTARY_PRESSED:
        // Increment hour, wrap at 24
        fsm->timeSet += MS_PER_HOUR;
        if (fsm->timeSet >= (time_t)HOURS_PER_DAY * MS_PER_HOUR) {
            fsm->timeSet = 0;
        }
        display_hhmm(fsm->timeSet, "Set hour");
        return RET_HANDLED;
    
    case ROTARY_CCW:
        // Decrement hour, wrap at 0
        if (fsm->timeSet < MS_PER_HOUR) {
            fsm->timeSet = (time_t)(HOURS_PER_DAY - 1) * MS_PER_HOUR;
        } else {
            fsm->timeSet -= MS_PER_HOUR;
        }
        display_hhmm(fsm->timeSet, "Set hour");
        return RET_HANDLED;

    case PUSH_PRESSED:
        // Confirm hour, move to minute setup
        fsm->state = state_uninitMin;
        return RET_TRANSITION;
    
    default:
        return RET_IGNORED;
    }
}

static fsm_return_status_t state_uninitMin(fsm_t * fsm, const event_t * e) {
    switch (e->signal) {
    case ENTRY:
        display_hhmm(fsm->timeSet, "Set minute");
        return RET_HANDLED;
    
    case ROTARY_PRESSED:
        {
            // Snapshot old hour block before adding
            time_t oldHourPart = (fsm->timeSet / MS_PER_HOUR) * MS_PER_HOUR;
            
            fsm->timeSet += MS_PER_MIN;
            
            // If the new hour part changed, it wrapped around!
            time_t newHourPart = (fsm->timeSet / MS_PER_HOUR) * MS_PER_HOUR;
            if (newHourPart != oldHourPart) {
                fsm->timeSet -= MS_PER_HOUR; // Revert the hour spillover
            }
        }
        display_hhmm(fsm->timeSet, "Set minute");
        return RET_HANDLED;
    
    case ROTARY_CCW:
        // Decrement hour, wrap at 0
        if (fsm->timeSet % MS_PER_HOUR < MS_PER_MIN) {
            fsm->timeSet += (time_t)(MINS_PER_HOUR - 1) * MS_PER_MIN;
        } else {
            fsm->timeSet -= MS_PER_MIN;
        }
        display_hhmm(fsm->timeSet, "Set minute");
        return RET_HANDLED;
    
    case PUSH_PRESSED:
        // Confirm - set system time and start clock
        scheduler_setTime(fsm->timeSet);
        fsm->state = state_normalOp;
        return RET_TRANSITION;
    
    default:
        return RET_IGNORED;
    }
}

static fsm_return_status_t state_normalOp(fsm_t * fsm, const event_t * e) {
    switch (e->signal) {
    case ENTRY:
        // Start green LED blink task
        display_time(scheduler_getTime());
        // Update yellow LED to reflect alarm status
        if (fsm->isAlarmEnabled) {
            led_yellowOn();
        } else {
            led_yellowOff();
        }
        return RET_HANDLED;
    
    case EXIT:
        // Green LED blink is handled by TICK_SECOND
        led_greenOff();
        return RET_HANDLED;
    
    case TICK_SECOND:
        // Update display & toggle green LED
        display_time(scheduler_getTime());
        led_greenToggle();
        return RET_HANDLED;
    
    case ROTARY_PRESSED:
        // Toggle alarm on/off
        fsm->isAlarmEnabled = !fsm->isAlarmEnabled;
        if (fsm->isAlarmEnabled) {
            led_yellowOn();
        } else {
            led_yellowOff();
        }
        return RET_HANDLED;
    
    case PUSH_PRESSED:
        // Enter alarm time setup
        fsm->state = state_setAlarmHour;
        return RET_TRANSITION;
    
    case ALARM_MATCH:
        fsm->state = state_alarmRinging;
        return RET_TRANSITION;
    
    default:
        return RET_IGNORED;
    }
}

static fsm_return_status_t state_setAlarmHour(fsm_t * fsm, const event_t * e) {
    switch (e->signal) {
    case ENTRY:
        fsm->timeSet = fsm->alarmTime;
        display_hhmm(fsm->timeSet, "Set alarm hour");
        return RET_HANDLED;
    
    case ROTARY_PRESSED:
        fsm->timeSet += MS_PER_HOUR;
        if (fsm->timeSet >= (time_t)HOURS_PER_DAY * MS_PER_HOUR) {
            fsm->timeSet = 0;
        }
        display_hhmm(fsm->timeSet, "Set alarm hour");
        return RET_HANDLED;
    
    case ROTARY_CCW:
        // Decrement hour, wrap at 0
        if (fsm->timeSet < MS_PER_HOUR) {
            fsm->timeSet = (time_t)(HOURS_PER_DAY - 1) * MS_PER_HOUR;
        } else {
            fsm->timeSet -= MS_PER_HOUR;
        }
        display_hhmm(fsm->timeSet, "Set alarm hour");
        return RET_HANDLED;

    case PUSH_PRESSED:
        fsm->state = state_setAlarmMin;
        return RET_TRANSITION;
    
    default:
        return RET_IGNORED;
    }
}

static fsm_return_status_t state_setAlarmMin(fsm_t * fsm, const event_t * e) {
    switch (e->signal) {
    case ENTRY:
        display_hhmm(fsm->timeSet, "Set alarm minute");
        return RET_HANDLED;
    
    case ROTARY_PRESSED:
        {
            // Snapshot old hour block before adding
            time_t oldHourPart = (fsm->timeSet / MS_PER_HOUR) * MS_PER_HOUR;
            
            fsm->timeSet += MS_PER_MIN;
            
            // If the new hour part changed, it wrapped around!
            time_t newHourPart = (fsm->timeSet / MS_PER_HOUR) * MS_PER_HOUR;
            if (newHourPart != oldHourPart) {
                fsm->timeSet -= MS_PER_HOUR; // Revert the hour spillover
            }
        }
        display_hhmm(fsm->timeSet, "Set alarm minute");
        return RET_HANDLED;
    
    case ROTARY_CCW:
        // Decrement hour, wrap at 0
        if (fsm->timeSet % MS_PER_HOUR < MS_PER_MIN) {
            fsm->timeSet += (time_t)(MINS_PER_HOUR - 1) * MS_PER_MIN;
        } else {
            fsm->timeSet -= MS_PER_MIN;
        }
        display_hhmm(fsm->timeSet, "Set alarm minute");
        return RET_HANDLED;
    
    case PUSH_PRESSED:
        // Save alarm time, return to normal - alarm NOT activated
        fsm->alarmTime = fsm->timeSet;
        fsm->state = state_normalOp;
        return RET_TRANSITION;
    
    default:
        return RET_IGNORED;
    }
}

static fsm_return_status_t state_alarmRinging(fsm_t * fsm, const event_t * e) {
    switch (e->signal) {
    case ENTRY:
        display_time(scheduler_getTime());

        // Start red LED 4 Hz flash task
        redFlashTask.task   = task_redFlash;
        redFlashTask.param  = NULL;
        redFlashTask.expire = RED_LED_FLASH_PERIOD;
        redFlashTask.period = RED_LED_FLASH_PERIOD;
        scheduler_add(&redFlashTask);

        // Start 5 sec auto-stop one-shot task
        alarmTimeoutTask.task    = task_alarmTimeout;
        alarmTimeoutTask.param   = NULL;
        alarmTimeoutTask.expire  = ALARM_DURATION_MS;
        alarmTimeoutTask.period  = 0;
        scheduler_add(&alarmTimeoutTask);
        return RET_HANDLED;
    
    case TICK_SECOND:
        // Keep updating the clock screen while the alarm is ringing!
        display_time(scheduler_getTime());
        return RET_HANDLED;
    
    case EXIT:
        // Stop flashing - remove tasks and turn LED off
        scheduler_remove(&redFlashTask);
        scheduler_remove(&alarmTimeoutTask);
        led_redOff();
        return RET_HANDLED;
    
    case PUSH_PRESSED:
    case ROTARY_PRESSED:
    case ALARM_TIMEOUT:
        // Stop alarm - return to normal, alarm remain enabled
        fsm->state = state_normalOp;
        return RET_TRANSITION;
    
    default:
        return RET_IGNORED;
    }
}

/* PUBLIC FUNCTIONS *********************************************************/

void alarmClock_init(void) {
    // Initialize FSM
    theFsm.isAlarmEnabled = false;
    theFsm.alarmTime = 0;
    theFsm.timeSet = 0;

    // Install button callbacks
    button_setRotaryButtonCallback(onRotaryPressed);
    button_setPushButtonCallback(onPushPressed);
    button_setRotaryCWCallback(onRotaryCW);
    button_setRotaryCCWCallback(onRotaryCCW);

    // Debounce task - every 5 ms
    debounceTask.task   = task_debounce;
    debounceTask.param  = NULL;
    debounceTask.expire = DEBOUNCE_PERIOD;
    debounceTask.period = DEBOUNCE_PERIOD;
    scheduler_add(&debounceTask);

    // Tick second task - every 1000 ms
    tickSecondTask.task     = task_tickSecond;
    tickSecondTask.param    = NULL;
    tickSecondTask.expire   = TICK_SECOND_PERIOD;
    tickSecondTask.period   = TICK_SECOND_PERIOD;
    scheduler_add(&tickSecondTask);

    // Alarm match check - every 1000 ms
    alarmCheckTask.task     = task_alarmCheck;
    alarmCheckTask.param    = NULL;
    alarmCheckTask.expire   = ALARM_CHECK_PERIOD;
    alarmCheckTask.period   = ALARM_CHECK_PERIOD;
    scheduler_add(&alarmCheckTask);

    // Start FSM in uninit hour state
    fsm_init(&theFsm, state_uninitHour);
}