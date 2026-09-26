#ifndef SES_FAN_H_
#define SES_FAN_H_

/* INCLUDES ******************************************************************/

#include <stdint.h>

/* FUNCTION PROTOTYPES *******************************************************/

/**
 * Initializes the fan PWM output on PC6 using Timer3 in Fast PWM 8-bit mode.
 * configures PD4 as output for the MOSFET power supply control.
 * PWM period is approx. 1 ms. Fan is disabled after initialization.
 */
void fan_init(void);

/**
 * Enables the fan by turning on the MOSFET power supply (PD4).
 */
void fan_enable(void);

/**
 * Disables the fan by turning off the MOSFET power supply (PD4)
 * and setting the duty cycle to zero.
 */
void fan_disable(void);

/**
 * Sets the PWM duty cycle for the fan speed/.
 * 
 * @param dc Duty cycle from 0 (slowest) to 255 (fastest).
 */
void fan_setDutyCycle(uint8_t dc);

#endif /* SES_FAN_H_ */