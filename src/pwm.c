#include "pwm.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

volatile uint16_t pwm_value = 0;
uint8_t min_pwm = 0;
uint8_t max_pwm = 255;

void pwm_init(void) {
    DDRB |= (1 << PB5);  // Set PB5 (OC1A) as output
    TCCR1A = (1 << COM1A1) | (1 << WGM11); 
    TCCR1B = (1 << WGM13) | (1 << CS10);    
    ICR1 = 1023;                            
    TIMSK1 |= (1 << TOIE1);                 
}

ISR(TIMER1_OVF_vect) {
    ADCSRA |= (1 << ADSC);  // Start ADC conversion on overflow
}

ISR(ADC_vect) {
    uint16_t adc_value = ADC;
    uint16_t scaled_min = ((uint32_t)min_pwm * 1023 + 127) / 255;
    uint16_t scaled_max = ((uint32_t)max_pwm * 1023 + 127) / 255;

    if (adc_value < scaled_min) adc_value = scaled_min;
    if (adc_value > scaled_max) adc_value = scaled_max;

    OCR1A = adc_value;
    pwm_value = adc_value;
}
