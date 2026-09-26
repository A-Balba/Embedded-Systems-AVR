/* INCLUDES *****************************************************************/

#include <avr/interrupt.h>
#include "ses_led.h"
#include "ses_button.h"
#include "ses_display.h"
#include "ses_scheduler.h"
#include "alarm_clock.h"

/* MAIN *********************************************************************/

int main(void) {
    // Hardware initializations
    led_greenInit();
    led_redInit();
    led_yellowInit();
    display_init();
    button_init(true);      // Debouncing via scheduler

    /* Scheduler must be initialized before alarmClock_init
       since it adds tasks immediately */
    scheduler_init();
    alarmClock_init();

    sei();
    scheduler_run();

    return 0;
}