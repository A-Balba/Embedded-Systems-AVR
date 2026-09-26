/* INCLUDES ******************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stddef.h>
#include <ses_button.h>
#include <ses_timer.h>

/* DEFINES & MACROS **********************************************************/

// Rotary button (press) & push button
#define BUTTON_ROTARY_DDR   DDRB
#define BUTTON_ROTARY_PORT  PORTB
#define BUTTON_ROTARY_PIN   PINB
#define BUTTON_ROTARY_BIT   5

#define BUTTON_PUSH_DDR     DDRB
#define BUTTON_PUSH_PORT    PORTB
#define BUTTON_PUSH_PIN     PINB
#define BUTTON_PUSH_BIT     4

// Rotary encoder A signal
#define ENCODER_A_DDR       DDRB
#define ENCODER_A_PORT      PORTB
#define ENCODER_A_PIN       PINB
#define ENCODER_A_BIT       7

// Rotary encoder B signal
#define ENCODER_B_DDR       DDRD
#define ENCODER_B_PORT      PORTD
#define ENCODER_B_PIN       PIND
#define ENCODER_B_BIT       6

// Debounce ring buffer
#define BUTTON_NUM_DEBOUNCE_CHECKS          5
#define BUTTON_DEBOUNCE_POS_PUSHBUTTON      0x01
#define BUTTON_DEBOUNCE_POS_ROTARYBUTTON    0x02

static volatile pButtonCallback rotaryButtonCallback = NULL;
static volatile pButtonCallback pushButtonCallback = NULL;
static volatile pButtonCallback rotaryCWCallback = NULL;
static volatile pButtonCallback rotaryCCWCallback = NULL;

/* FUNCTION DEFINITION *******************************************************/

void button_init(bool debouncing) {
    // Push and rotary buttons as inputs with pull-ups
    BUTTON_ROTARY_DDR   &= ~(1 << BUTTON_ROTARY_BIT);
    BUTTON_PUSH_DDR     &= ~(1 << BUTTON_PUSH_BIT);
    BUTTON_ROTARY_PORT  |= (1 << BUTTON_ROTARY_BIT);
    BUTTON_PUSH_PORT    |= (1 << BUTTON_PUSH_BIT);

    // Encoder A as input with pull-up
    ENCODER_A_DDR &= ~(1 << ENCODER_A_BIT);
    ENCODER_A_PORT |= (1 << ENCODER_A_BIT);

    // Encoder B as input with pull-up
    ENCODER_B_DDR &= ~(1 << ENCODER_B_BIT);
    ENCODER_B_PORT |= (1 << ENCODER_B_BIT);

    if (!debouncing) {
        // Poll button state every 5 ms via scheduler
        PCICR |= (1 << PCIE0);
        PCMSK0 |= (1 << BUTTON_ROTARY_BIT) | (1 << BUTTON_PUSH_BIT);
    }
}

bool button_isPushButtonPressed(void) {
    // Buttons are active-low with pull-ups
    return !(BUTTON_PUSH_PIN & (1 << BUTTON_PUSH_BIT));
}

bool button_isRotaryButtonPressed(void) {
    // Buttons are active-low with pull-ups
    return !(BUTTON_ROTARY_PIN & (1 << BUTTON_ROTARY_BIT));
}

void button_setRotaryButtonCallback(pButtonCallback callback) {
    rotaryButtonCallback = callback;
}

void button_setPushButtonCallback(pButtonCallback callback) {
    pushButtonCallback = callback;
}

void button_setRotaryCWCallback(pButtonCallback callback) {
    rotaryCWCallback = callback;
}

void button_setRotaryCCWCallback(pButtonCallback callback) {
    rotaryCCWCallback = callback;
}

/* DEBOUNCE ******************************************************************/
void button_checkState(void) {
    // Push and rotary buttons debounce
    static uint8_t state[BUTTON_NUM_DEBOUNCE_CHECKS] = { 0 };
    static uint8_t index            = 0;
    static uint8_t debounceState    = 0;
    uint8_t lastDebounceState       = debounceState;

    // Sample buttons into 1 byte, 1 bit each
    state[index] = 0;
    if (button_isPushButtonPressed()) {
        state[index] |= BUTTON_DEBOUNCE_POS_PUSHBUTTON;
    }
    if (button_isRotaryButtonPressed()) {
        state[index] |= BUTTON_DEBOUNCE_POS_ROTARYBUTTON;
    }

    // Advance ring buffer index
    index++;
    if (index == BUTTON_NUM_DEBOUNCE_CHECKS) {
        index = 0;
    }

    // Button is pressed only when all stored samples agree
    uint8_t j = 0xFF;
    for (uint8_t i = 0; i < BUTTON_NUM_DEBOUNCE_CHECKS; i++) {
        j = j & state[i];
    }
    debounceState = j;

    // Fire callbacks on press not on release
    uint8_t pressed = debounceState & ~lastDebounceState;

    if ((pressed & BUTTON_DEBOUNCE_POS_PUSHBUTTON) && pushButtonCallback != NULL) {
        pushButtonCallback();
    }
    if ((pressed & BUTTON_DEBOUNCE_POS_ROTARYBUTTON) && rotaryButtonCallback != NULL) {
        rotaryButtonCallback();
    }

    // Rotary encoder CW/CCW detection

    /**
     * We sample signal A every 5 ms. When A transitions from low to high
     * (rising edge), we read B to determine direction:
     * B = low -> CW (A leads B)
     * B = high -> CCW (B leads A)
     */
    static uint8_t lastA = 1;   // Last sampled state of A

    uint8_t currentA = (ENCODER_A_PIN >> ENCODER_A_BIT) & 0x01;
    uint8_t currentB = (ENCODER_B_PIN >> ENCODER_B_BIT) & 0x01;

    // Detect rising edge of A (low to high)
    if (currentA == 1 && lastA == 0) {
        if (currentB == 0) {
            // B is low when A rises -> CW
            if (rotaryCWCallback != NULL) {
                rotaryCWCallback();
            }
        } else {
            // B is high when A rises -> CCW
            if (rotaryCCWCallback != NULL) {
                rotaryCCWCallback();
            }
        }
    }

    lastA = currentA;
}

/* INTERRUPT SERVICE ROUTINE *************************************************/

ISR(PCINT0_vect)
{
    static uint8_t lastPinState = 0xFF; // all buttons not pressed initially

    uint8_t currentPinState = BUTTON_ROTARY_PIN; // Same port for both buttons
    uint8_t changed = lastPinState ^ currentPinState;
    lastPinState = currentPinState;

    // Is Rotary button pressed?
    if ((changed & (1 << BUTTON_ROTARY_BIT)) &&
        (PCMSK0 & (1 << BUTTON_ROTARY_BIT)) &&
        !(currentPinState & (1 << BUTTON_ROTARY_BIT)) &&
        rotaryButtonCallback != NULL)
    {
        rotaryButtonCallback();
    }

    // Is Push button pressed?
    if ((changed & (1 << BUTTON_PUSH_BIT)) &&
        (PCMSK0 & (1 << BUTTON_PUSH_BIT)) &&
        !(currentPinState & (1 << BUTTON_PUSH_BIT)) &&
        pushButtonCallback != NULL)
    {
        pushButtonCallback();
    }
}