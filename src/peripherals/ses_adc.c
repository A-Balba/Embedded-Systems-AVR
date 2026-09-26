/*INCLUDES *******************************************************************/

#include <avr/io.h>
#include <inttypes.h>
#include <ses_adc.h>

/* DEFINES & MACROS **********************************************************/

// Port F pin assigns
#define ADC_LIGHT_BIT   0   // Light sensor
#define ADC_POTI_BIT   6   //Potentiometer
#define ADC_tEMP_BIT  7   // Temperature Sensor

// Prescaler: 16 MHz / 128 = 125 kHz
#define ADC_PRESCALE   7

/* FUNCTION DEFINITION *******************************************************/

void adc_init(void) {
    // Setting potentiometer, temperature, light as inputs
    DDRF &= ~((1 << ADC_LIGHT_BIT) | (1 << ADC_POTI_BIT) | (1 << ADC_tEMP_BIT));

    // Deactivating pull-up resistors for ADC pins
    PORTF &= ~((1 << ADC_LIGHT_BIT) | (1 << ADC_POTI_BIT) | (1 << ADC_tEMP_BIT));

    // Disable power reduction for ADC: clear PRADC IN PRR0
    PRR0 &= ~(1 << PRADC);
    
    // Select external reference voltage (AREF): clear REFS1 REFS0
    ADMUX &= ~((1 << REFS1) | (1 << REFS0));

    // Right-adjust result: clear ADLAR
    ADMUX &= ~((1 << ADLAR));

    // Set prescale bits ADPS2: ADPS0 = 111 (128 factor)
    ADCSRA = (ADCSRA & ~ 0x07) | (ADC_PRESCALE & 0x07);

    // Disable auto triggering
    ADCSRA &= ~(1 << ADATE);

    // Enable ADC
    ADCSRA |= (1 << ADEN);
}

uint16_t adc_read(uint8_t adc_channel) {
    // Validate channel
    if (adc_channel != ADC_LIGHT_CH && adc_channel != ADC_POTI_CH && adc_channel != ADC_TEMP_CH) {
        return ADC_INVALID_CHANNEL;
    }

    // Select channel in ADMUX, preserve upper 3 bits
    ADMUX = (ADMUX & 0xE0) | (adc_channel & 0x1F);

    // Start single conversion
    ADCSRA |= (1 << ADSC);

    // Wait for conversion to complete (polling)
    while (ADCSRA & (1 << ADSC));

    // Read result: ADCL --> ADCH
    uint8_t low = ADCL;
    uint8_t high = ADCH;

    return ((uint16_t)high << 8) | low;
}

int16_t adc_getTemperature(void) {
    // Read raw ADC value from temperature channel
    int32_t adc = adc_read(ADC_TEMP_CH);

    return (int16_t)(((adc - ADC_TEMP_RAW_LOW) * (ADC_TEMP_HIGH - ADC_TEMP_LOW)) / (ADC_TEMP_RAW_HIGH - ADC_TEMP_RAW_LOW)) + ADC_TEMP_LOW;
}