#ifndef SES_BUTTON_H_
#define SES_BUTTON_H_

/* INCLUDES ******************************************************************/

#include <stdbool.h>
#include <stddef.h>

/* TYPEDEFS ******************************************************************/

typedef void (*pButtonCallback)(void);

/* FUNCTION PROTOTYPES *******************************************************/

/**
 * Initializes rotary encoder button and pushbutton
 * @param debouncing true = scheduler-based debouncing
 *                   false = direct pin-change interrupts
 */
void button_init(bool debouncing);

/** 
 * Get the state of the pushbutton.
 */
bool button_isPushButtonPressed(void);

/** 
 * Get the state of the rotary button.
 */
bool button_isRotaryButtonPressed(void);

/** 
 * Installs a callback function for the rotary button.
 * The callback is invoked once per button press (falling edge).
 * Pass NULL to disable the callback.
 * 
 * @param callback Function pointer to invoke on press, or NULL.
 */
void button_setRotaryButtonCallback(pButtonCallback callback);

/** 
 * Installs a callback function for the push button.
 * The callback is invoked once per button press (falling edge).
 * Pass NULL to disable the callback.
 * 
 * @param callback Function pointer to invoke on press, or NULL.
 */
void button_setPushButtonCallback(pButtonCallback callback);

/**
 * Installs a callback for clockwise rotation of the rotary encoder.
 * Pass NULL to disable.
 * @param callback Function pointer to invoke on CW turn, or NULL.
 */
void button_setRotaryCWCallback(pButtonCallback callback);

/**
 * Installs a callback for counterclockwise rotation of the rotary encoder.
 * Pass NULL to disable.
 * @param callback Function pointer to invoke on CCW turn, or NULL.
 */
void button_setRotaryCCWCallback(pButtonCallback callback);

/**
 * Samples both buttons and fires callbacks after
 * BUTTON_NUM_DEBOUNCE_CHECKS consistent reads.
 * Call periodically every 5 ms
 */
void button_checkState(void);


#endif /* SES_BUTTON_H_ */
