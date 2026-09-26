/* INCLUDES ******************************************************************/
#include "ses_timer.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stddef.h>

/* DEFINES & MACROS **********************************************************/
#define TIMER0_PRESCALER			64UL
#define TIMER0_INTERRUPT_HZ			1000UL
#define TIMER0_CYC_FOR_1MILLISEC	((F_CPU / (TIMER0_PRESCALER * TIMER0_INTERRUPT_HZ)) - 1)

#define TIMER1_PRESCALER			64UL
#define TIMER1_INTERRUPT_HZ			200UL
#define TIMER1_CYC_FOR_5MILLISEC	((F_CPU / (TIMER1_PRESCALER * TIMER1_INTERRUPT_HZ)) - 1)

static volatile pTimerCallback timer0Callback = NULL;
static volatile pTimerCallback timer1Callback = NULL;

/*FUNCTION DEFINITION ********************************************************/
void timer0_setCallback(pTimerCallback cb) {
	timer0Callback = cb;
}

void timer0_start() {
	// CTC mode: WGM01=1, WGM00=0
	TCCR0A = (1 << WGM01);

	/*Set compare value for 1 ms:
	  OCR0A = (16000000 / (64 * 1000)) - 1 = 249 */
	OCR0A = (uint8_t)TIMER0_CYC_FOR_1MILLISEC;

	// Clear interrupt flag before enabling 
	TIFR0 |= (1 << OCF0A);

	// Enable Output Compare Match A interrupt
	TIMSK0 |= (1 << OCIE0A);

	// Set prescaler 64
	TCCR0B = (1 << CS01) | (1 << CS00);
}


void timer0_stop() {
    // Clear prescaler bits to stop timer clock
	TCCR0B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));

	// Disable the compare match interrupt
	TIMSK0 &= ~(1 << OCIE0A);
}

void timer1_setCallback(pTimerCallback cb) {
	timer1Callback = cb;
}


void timer1_start() {
	TCCR1A = 0;
	TCCR1B = (1 << WGM12);

	// Set compare value for 5 ms
	OCR1A = (uint16_t)TIMER1_CYC_FOR_5MILLISEC;

	// Clear interrupt flag
	TIFR1 |= (1 << OCF1A);

	// Enable output compare match A interrupt
	TIMSK1 |= (1 << OCIE1A);

	// Set prescaler
	TCCR1B |= (1 << CS11) | (1 << CS10);
}


void timer1_stop() {
	// Clear prescaler bits to stop timer clock
	TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));

	// Disable the compare match interrupt
	TIMSK1 &= ~(1 << OCIE1A);
}

/*INTERRUPT SERVICE ROUTINES ****************************************************/
ISR(TIMER0_COMPA_vect) {
	if (timer0Callback != NULL) {
		timer0Callback();
	}
}

ISR(TIMER1_COMPA_vect) {
	if (timer1Callback != NULL) {
		timer1Callback();
	}
}
