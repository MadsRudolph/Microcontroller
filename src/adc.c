#include <avr/io.h>
#include <avr/interrupt.h>
#include "adc.h"

volatile uint16_t pwm_value = 0;
uint8_t min_pwm = 0;
uint8_t max_pwm = 255;

ISR(ADC_vect) {
    uint16_t adc_value = ADC;
    uint16_t scaled_min = ((uint32_t)min_pwm * 1023 + 127) / 255;
    uint16_t scaled_max = ((uint32_t)max_pwm * 1023 + 127) / 255;

    if (adc_value < scaled_min) adc_value = scaled_min;
    if (adc_value > scaled_max) adc_value = scaled_max;

    OCR1A = adc_value;
    pwm_value = adc_value;
}

void adc_init(void) {
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN)  |
             (1 << ADIE)  |
             (1 << ADATE) |
             (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}
