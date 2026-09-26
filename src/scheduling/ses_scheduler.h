#ifndef SCHEDULER_H_
#define SCHEDULER_H_

/* INCLUDES *****************************************************************/

#include <stdbool.h>
#include <stdint.h>

/* TYPES ********************************************************************/

/**
 * Type of function pointer for tasks
 */
typedef void (* task_t)(void *);

/**
 * time_t: milliseconds since midnight.
 * 24h * 60min * 60s * 100ms = 86400000 ms
 */
typedef uint32_t time_t;

/**
 * Human-readable time structure
 */
struct tm {
   int16_t  tm_millis;
   int8_t   tm_sec;
   int8_t   tm_min;
   int8_t   tm_hour;
};

/**
 * Task structure to schedule tasks
 */
typedef struct task_descriptor_s {
   task_t task;          ///< function pointer to call
   void *  param;        ///< pointer, which is passed to task when executed
   uint16_t expire;      ///< time offset in ms, after which to call the task
   uint16_t period;      ///< period of the timer after firing; 0 means exec once
   uint8_t execute:1;    ///< for internal use
   uint8_t reserved:7;   ///< reserved
   struct task_descriptor_s * next; ///< pointer to next task, internal use
} task_descriptor_t;


/* FUNCTION PROTOTYPES *******************************************************/

/**
 * Initializes the task scheduler and its associated timer
 */
void scheduler_init(void);

/**
 * Runs scheduler in an infinite loop.
 */
void scheduler_run(void);

/**
 * Adds a new task to the scheduler.
 *
 * @param td   Pointer to task_descriptor_t structure. The scheduler takes
 *             possesion of the memory pointed at by td until the task
 *             is removed by scheduler_remove. 
 *             One-shot task are removed automatically.
 *             td->expire and td->execute are used by the scheduler 
 *             to determine when to execute task.
 *
 * @return     false, if task is already present or invalid (NULL)
 *             true, if task was successfully added to scheduler and will be
 *             scheduled after td->expire ms
 */
bool scheduler_add(task_descriptor_t * td);

/**
 * Removes a task from the scheduler.
 *
 * @param td	Pointer to task descriptor to remove
 * */
void scheduler_remove(const task_descriptor_t * td);

/**
 * Returns the current system time as milliseconds since midnight.
 * @return Current time in milliseconds.
 */
time_t scheduler_getTime(void);

/**
 * Sets the current system time.
 * @param time Time in milliseconds since midnight.
 */
void scheduler_setTime(time_t time);

/**
 * Converts time_t to human-readable struct tm.
 * @param t Time in milliseconds since midnight.
 * @param tm Pointer to struct tm to fill.
 */
void scheduler_toTm(time_t t, struct tm * tm);

/**
 * Converts human-readable struct tm to time_t.
 * @param tm Pointer to struct tm to convert.
 * @return Time in milliseconds since midnight.
 */
time_t scheduler_fromTm(const struct tm * tm);

#endif /* SCHEDULER_H_ */