#include <avr/io.h>
#include <avr/interrupt.h>
#include "adc.h"

// Global variables for PWM limits and ADC value
volatile uint16_t pwm_value = 0;  // Store ADC result
uint8_t min_pwm = 0;              // Minimum PWM limit
uint8_t max_pwm = 255;            // Maximum PWM limit

// ADC interrupt service routine: clamp ADC result and set PWM
ISR(ADC_vect) {
    uint16_t adc_value = ADC;  // Read ADC result
    uint16_t scaled_min = ((uint32_t)min_pwm * 1023 + 127) / 255;  // Scale MIN PWM
    uint16_t scaled_max = ((uint32_t)max_pwm * 1023 + 127) / 255;  // Scale MAX PWM

    // Clamp ADC value within the min and max limits
    if (adc_value < scaled_min) adc_value = scaled_min;
    if (adc_value > scaled_max) adc_value = scaled_max;

    OCR1A = adc_value;  // Update PWM duty cycle
    pwm_value = adc_value;  // Store value for OLED display
}

// Initialize ADC with proper configuration
void adc_init(void) {
    ADMUX = (1 << REFS0);  // Set AVcc as reference voltage, ADC0 as input
    ADCSRA = (1 << ADEN)  |  // Enable ADC
             (1 << ADIE)  |  // Enable ADC interrupt
             (1 << ADATE) |  // Enable auto-triggering
             (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // Set prescaler to 128 (125kHz ADC clock)
}
