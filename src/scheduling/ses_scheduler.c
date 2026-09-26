/* INCLUDES ******************************************************************/
#include <stdlib.h>
#include <avr/interrupt.h>
#include "ses_timer.h"
#include "ses_scheduler.h"
#include "util/atomic.h"

/* DEFINES & MACROS **********************************************************/

#define MS_PER_DAY  (24UL * 60UL * 60UL * 1000UL)   // 86400000 ms
#define MS_PER_HOUR (60UL * 60UL * 1000UL)          // 3600000 ms
#define MS_PER_MIN  (60UL * 1000UL)                 // 60000 ms
#define MS_PER_SEC  (1000UL)                        // 1000 ms

/* PRIVATE VARIABLES *********************************************************/

/**
 * We make sure that the list is accessed only within atomic sections
 * protected by a memory barrier --> no volatile necessary
 */
static task_descriptor_t * taskList = NULL;
static time_t systemTime = 0;   // ms since midnight


/* FUNCTION DEFINITION *******************************************************/

static void scheduler_update(void) {
    // Increment system clock
    systemTime++;
    if (systemTime >= MS_PER_DAY) {
        systemTime = 0;
    }

    // Decrement expire for all tasks & mark expired ones
    task_descriptor_t * current = taskList;

    while (current != NULL) {
        if (current->expire > 0) {
            current->expire--;
        }
        if (current->expire == 0 && current->execute == 0) {
            current->execute = 1;

            if (current->period > 0) {
                current->expire = current->period;
            }
        }
        current = current->next;
    }
}

void scheduler_init(void) {
    taskList = NULL;
    systemTime = 0;
    timer0_setCallback(scheduler_update);
    timer0_start();
}

time_t scheduler_getTime(void) {
    time_t t;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = systemTime;
    }

    return t;
}

void scheduler_setTime(time_t time) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        systemTime = time % MS_PER_DAY;
    }
}

void scheduler_toTm(time_t t, struct tm * tm) {
    tm->tm_hour     = (int8_t)(t / MS_PER_HOUR);
    t               %= MS_PER_HOUR;
    tm->tm_min      = (int8_t)(t / MS_PER_MIN);
    t               %= MS_PER_MIN;
    tm->tm_sec      = (int8_t)(t / MS_PER_SEC);
    tm->tm_millis   = (int16_t)(t % MS_PER_SEC);
}

time_t scheduler_fromTm(const struct tm * tm) {
    return (time_t)tm->tm_hour  * MS_PER_HOUR
         + (time_t)tm->tm_min   * MS_PER_MIN
         + (time_t)tm->tm_sec   * MS_PER_SEC
         + (time_t)tm->tm_millis;
}

void scheduler_run(void) {
    while (1) {
        task_descriptor_t * current;

        // Atomically get head of list
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            current = taskList;
        }

        while (current != NULL) {
            uint8_t shouldExecute = 0;
            task_descriptor_t * next;

            // Atomically check and clear the execute flag
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                next = current->next;
                if (current->execute) {
                    shouldExecute = 1;
                    current->execute = 0;
                }
            }

            if (shouldExecute) {
                // Execute task outside atomic block
                current->task(current->param);

                // Remove one-shot tasks after execution
                if (current->period == 0) {
                    scheduler_remove(current);
                }
            }

            current = next;
            
        }
    }
}

bool scheduler_add(task_descriptor_t * toAdd) {
    // Reject NULL or invalid task
    if (toAdd == NULL || toAdd->task == NULL) {
        return false;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        // Check for duplicates
        task_descriptor_t * current = taskList;
        while (current != NULL) {
            if (current == toAdd) {
                return false;   // Already in list
            }
            current = current->next;
        }

        // Insert at head - 0(1)
        toAdd->execute = 0;
        toAdd->next = taskList;
        taskList = toAdd;
    }

    return true;

}

void scheduler_remove(const task_descriptor_t * toRemove) {
    if (toRemove == NULL) {
        return;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (taskList == NULL) {
            return;
        }
            
        // Special case: removing the head
        if (taskList == toRemove) {
            taskList = taskList->next;
            return;
        }

        // Find the node before toRemove
        task_descriptor_t * prev = taskList;
        while (prev->next != NULL) {
            if (prev->next == toRemove) {
                prev->next = toRemove->next;
                return;
            }
            prev = prev->next;
        }
        // toRemove not found then do nothing
    }
}
