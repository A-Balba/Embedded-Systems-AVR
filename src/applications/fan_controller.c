/* INCLUDES ******************************************************************/

#include <avr/interrupt.h>
#include <stdio.h>
#include <stdint.h>
#include "ses_led.h"
#include "ses_button.h"
#include "ses_adc.h"
#include "ses_fan.h"
#include "ses_fanspeed.h"
#include "ses_scheduler.h"
#include "ses_display.h"

/* DEFINES & MACROS ***********************************************************/

#define POTI_READ_PERIOD    50      // Read potentiometer every 50 ms
#define DEBOUNCE_PERIOD     5       // Button debounce every 5 ms
#define DISPLAY_PERIOD      1000    // Refresh display every 1 s

/* TASK DESCRIPTORS ***********************************************************/

static task_descriptor_t potiTask;
static task_descriptor_t debounceTask;
static task_descriptor_t displayTask;

/* GLOBAL STATE ***************************************************************/

static bool fanOn               = false;
static uint8_t currentDutyCycle = 0;

/* TASK FUNCTIONS *************************************************************/

static void task_readPoti(void * param) {
    (void)param;

    // Map 10-bit ADC to 8-bit duty cycle
    uint16_t raw = adc_read(ADC_POTI_CH);
    currentDutyCycle = (uint8_t)(raw >> 2);

    if (fanOn) {
        fan_setDutyCycle(currentDutyCycle);
    }
}

static void task_debounce(void * param) {
    (void)param;
    button_checkState();
}

static void task_updateDisplay(void * param) {
    (void)param;

    uint16_t rpm = fanspeed_getRecent();
    uint16_t rpmFiltered = fanspeed_getFiltered();
    uint8_t dc = (currentDutyCycle * 100U) / 255U;

    display_clear();
    display_setCursor(0, 0);
    fprintf(displayout, "%u/%u RPM", rpm, rpmFiltered);
    display_setCursor(0, 1);
    fprintf(displayout, "DC: %u%%", dc);
    display_update();
}

/* BUTTON CALLBACKS *************************************************************/

static void onPushButton(void) {
    if (fanOn) {
        fan_disable();
        fanOn = false;
    } else {
        fan_enable();
        fan_setDutyCycle(currentDutyCycle);
        fanOn = true;
    }
}

/* MAIN *************************************************************************/

int main(void) {
    // Hardware initializations
    led_redInit();
    led_greenInit();
    led_yellowInit();
    display_init();
    adc_init();
    fan_init();
    fanspeed_init();
    button_init(true);

    button_setPushButtonCallback(onPushButton);

    scheduler_init();

    // Read potentiometer every 50 ms
    potiTask.task   = task_readPoti;
    potiTask.param  = NULL;
    potiTask.expire = POTI_READ_PERIOD;
    potiTask.period = POTI_READ_PERIOD;
    scheduler_add(&potiTask);

    // Debounce buttons every 5 ms
    debounceTask.task   = task_debounce;
    debounceTask.param  = NULL;
    debounceTask.expire = DEBOUNCE_PERIOD;
    debounceTask.period = DEBOUNCE_PERIOD;
    scheduler_add(&debounceTask);

    // Refresh display every 1 s
    displayTask.task    = task_updateDisplay;
    displayTask.param   = NULL;
    displayTask.expire  = DISPLAY_PERIOD;
    displayTask.period  = DISPLAY_PERIOD;
    scheduler_add(&displayTask);

    sei();
    scheduler_run();

    return 0;
}