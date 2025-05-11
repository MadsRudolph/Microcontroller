#ifndef ADC_H
#define ADC_H

#include <stdint.h>

// External variables for PWM limits
extern volatile uint16_t pwm_value;
extern uint8_t min_pwm;
extern uint8_t max_pwm;

// Function to initialize ADC
void adc_init(void);

#endif
