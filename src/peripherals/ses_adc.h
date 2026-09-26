#ifndef SES_ADC_H
#define SES_ADC_H

/*INCLUDES *******************************************************************/

#include <inttypes.h>
#include <avr/io.h>


/* DEFINES & MACROS **********************************************************/

/* to signal that the given channel was invalid */
#define ADC_INVALID_CHANNEL    0xFFFF

// Temperature interpulation constans
#define ADC_TEMP_LOW  100 // 10.0°C in tenth of °C
#define ADC_TEMP_HIGH 300 // 30.0°C in tenth of °C
#define ADC_TEMP_RAW_LOW  853 // raw ADC value 10.0°C
#define ADC_TEMP_RAW_HIGH 551 // raw ADC value 30.0°C

enum ADCChannels {
  ADC_LIGHT_CH = 0,        /* ADC0 */
  ADC_POTI_CH  = 6,        /* ADC6 */
  ADC_TEMP_CH  = 7,        /* ADC7 */
};


/* FUNCTION PROTOTYPES *******************************************************/

/**
 * Initializes the ADC
 */
void adc_init(void);


/**
 * Read the raw ADC value of the given channel
 * @adc_channel The channel as element of the ADCChannels enum
 * @return The raw ADC value
 */
uint16_t adc_read(uint8_t adc_channel);

/**
 * Read the current temperature
 * @return Temperature in tenths of degree celsius
 */
int16_t adc_getTemperature(void);

#endif /* SES_ADC_H */
