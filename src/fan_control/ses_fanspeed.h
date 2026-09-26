#ifndef SES_FANSPEED_H_
#define SES_FANSPEED_H_

/* INCLUDES ******************************************************************/

#include <stdint.h>

/* FUNCTION PROTOTYPES *******************************************************/

/**
 * Initializes fan speed measurement using Timer1 and INT6.
 * Timer1 runs as a free-running counter with prescaler 64.
 * INT6 fires on each rising edge of the tacho signal (PE6).
 * Two rising edges = one rev.
 */
void fanspeed_init(void);

/**
 * Returns the most recent fan speed estimate in RPM.
 * Returns 0 if the fan is stopped or no pulse has been detected.
 * @return Fan speed in RPM.
 */
uint16_t fanspeed_getRecent(void);

/**
 * Returns the median-filtered fan speed in RPM.
 * Uses a ring buffer of the last N samples.
 * @return Filtered fan speed in RPM.
 */
uint16_t fanspeed_getFiltered(void);

#endif /* SES_FANSPEED_H_ */