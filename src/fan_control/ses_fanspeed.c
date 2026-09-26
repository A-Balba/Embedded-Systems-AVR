/* INCLUDES ******************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdint.h>
#include "ses_fanspeed.h"
#include "ses_led.h"

/* DEFINES & MACROS ***********************************************************/

#define FAN_TACHO_DDR       DDRE
#define FAN_TACHO_PORT      PORTE
#define FAN_TACHO_BIT       6

/**
 * Timer1 presclaer 64:
 *  tick period = 64 / 16MHz = 4 us
 *  max measurable period before overflow = 4us * 65536 = ~262 ms
 *  at min fan speed ~1000 RPM:
 *      period per rev. = 60s / 1000 = 60 ms
 *      period per pulse (2 pulses/rev) = 30 ms -> within range
 * 
 * RPM from pulse period:
 *  pulse period [s] = ticks * 4us
 *  pulses/rev = 2
 *  RPM = 60 / (2 * pulse_period) = 7500000 / ticks
 */
#define TIMER1_PRESCALER    64UL
#define TIMER1_TICK_US      (TIMER1_PRESCALER * 1000000UL / F_CPU)
#define RPM_NUMERATOR       (60UL * 1000000UL / (2UL * TIMER1_TICK_US))

// Median filter size (odd no. guarantees true middle elem.)
#define FILTER_SIZE         7

/* PRIVATE VARIABLES **********************************************************/

static volatile uint16_t recentRPM = 0;
static volatile uint16_t lastCapture = 0;

// Median filter ring buffer
static volatile uint16_t filterBuf[FILTER_SIZE] = {0};
static volatile uint8_t filterIndex = 0;

/* PRIVATE FUNCTIONS **********************************************************/

// Inserts a new RPM sample into the ring buffer
static void filter_insert(uint16_t rpm) {
    filterBuf[filterIndex] = rpm;
    filterIndex++;
    if (filterIndex >= FILTER_SIZE) {
        filterIndex = 0;
    }
}

// Sorts dst array length using insertion sort
static void sort_copy(uint16_t * dst, const uint16_t * src, uint8_t len) {
    // copy src into dst
    for (uint8_t i = 0; i < len; i++) {
        dst[i] = src[i];
    }

    // Insertion sort
    for (uint8_t i = 1; i < len; i ++) {
        uint16_t key = dst[i];
        int8_t j = (int8_t)i - 1;

        while (j >= 0 && dst[j] > key) {
            dst[j + 1] = dst[j];
            j--;
        }
        dst[j + 1] = key;
    }
}

/* FUNCTION DEFINITION *********************************************************/

void fanspeed_init(void) {
    // PE6 as input, no pull-up
    FAN_TACHO_DDR &= ~(1 << FAN_TACHO_BIT);
    FAN_TACHO_PORT &= ~(1 << FAN_TACHO_BIT);

    //Timer1 free-running with prescaler 64
    TCCR1A = 0;
    TCCR1B = (1 << CS11) | (1 << CS10);
    TCNT1 = 0;

    // Enable Timer1 overflow interrupt for stopped-fan detection
    TIMSK1 |= (1 << TOIE1);

    // Configure INT6 on rising edge
    EICRB |= (1 << ISC61) | (1 << ISC60);
    EIMSK |= (1 << INT6);
}

uint16_t fanspeed_getRecent(void) {
    uint16_t rpm;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        rpm = recentRPM;
    }

    return rpm;
}

uint16_t fanspeed_getFiltered(void) {
    uint16_t localBuf[FILTER_SIZE];

    // Atomically copy ring buffer to avoid race with INT6 ISR
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        for (uint8_t i = 0; i < FILTER_SIZE; i++) {
            localBuf[i] = filterBuf[i];
        }
    }

    // Sort the local copy
    sort_copy(localBuf, localBuf, FILTER_SIZE);

    // Return middle element
    return localBuf[FILTER_SIZE / 2];
}

/* INTERRUPT SERVICE ROUTINES ******************************************************/

/**
 * INT6 Fires on each rising edge of the tacho signal.
 * Measures elapsed ticks since last pulse and computes RPM.
 * Two rising edges = one full rev.
 */
ISR(INT6_vect) {
    uint16_t now = TCNT1;
    uint16_t elapsed = now - lastCapture;
    lastCapture = now;

    if (elapsed > 0) {
        recentRPM = (uint16_t)(RPM_NUMERATOR / elapsed);
        filter_insert(recentRPM);
    }

    // Fan is running, turn off stopped indicator
    led_redOff();
}

/**
 * Timer1 overflow fires every ~262 ms if no tacho pulse is recieved.
 * If triggers, the fan has stopped or is running below ~229 RPM.
 */
ISR(TIMER1_OVF_vect) {
    recentRPM = 0;
    filter_insert(0);

    // Signal stopped fan with red LED
    led_redOn();
}