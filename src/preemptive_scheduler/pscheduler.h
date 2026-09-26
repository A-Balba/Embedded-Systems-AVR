#ifndef PSCHEDULER_H_
#define PSCHEDULER_H_

#include <stdint.h>

/* task_t for preemptive scheduler (no parameter, runs forever)*/
typedef void (* task_t)(void);

/**
 * Initializes and runs preemptive scheduler using round-robin
 * Never returns.
 * 
 * @param taskArray Array of task function pointers
 * @param len       Number of tasks
 */
void pscheduler_run(const task_t * taskArray, uint8_t len);

#endif /* PSCHEDULER_H_ */