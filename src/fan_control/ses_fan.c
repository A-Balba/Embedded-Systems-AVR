/* INCLUDES ******************************************************************/

#include <avr/io.h>
#include "ses_fan.h"

/* DEFINES & MACROS **********************************************************/

// PD4 controls the MOSFET (fan power supply)
#define FAN_MOSFET_DDR      DDRD
#define FAN_MOSFET_PORT     PORTD
#define FAN_MOSFET_BIT      4

// PC6 is the PWM output (Timer3 OC3A)
#define FAN_PWM_DDR         DDRC
#define FAN_PWM_BIT         6

#define FAN_TCCR3A_VAL  ((1 << COM3A1) | (0 << COM3A0) | \
                         (0 << WGM31)  | (1 << WGM30))
#define FAN_TCCR3B_VAL  ((0 << WGM33) | (1 << WGM32) | \
                         (1 << CS31)  | (1 << CS30))    // prescaler 64

/* FUNCTION DEFINITION ********************************************************/

void fan_init(void) {
    // Configure PD4 as output for MOSFET control
    FAN_MOSFET_DDR |= (1 << FAN_MOSFET_BIT);

    // Configure PC6 as output for PWM signal
    FAN_PWM_DDR |= (1 << FAN_PWM_BIT);

    // Enable Timer3 by clearing power-reduction bit
    PRR1 &= ~(1 << PRTIM3);

    // Configure Fast PWM 8-bit, non-inverting on OC3A
    TCCR3A = FAN_TCCR3A_VAL;
    TCCR3B = FAN_TCCR3B_VAL;
    TCCR3C = 0;

    // Initialize duty cycle to 0
    OCR3A = 0;

    // Fan off by default
    fan_disable();
}

void fan_enable(void) {
    // Turn on MOSFET (active high)
    FAN_MOSFET_PORT |= (1 << FAN_MOSFET_BIT);
}

void fan_disable(void) {
    // Set duty cycle to zero before cutting power
    OCR3A = 0;

    // Turn off MOSFET
    FAN_MOSFET_PORT &= ~(1 << FAN_MOSFET_BIT);
}

void fan_setDutyCycle(uint8_t dc) {
    OCR3A = dc;
}