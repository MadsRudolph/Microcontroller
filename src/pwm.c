#include <avr/io.h>
#include <avr/interrupt.h>
#include "pwm.h"

// Initialize Timer1 for PWM operation
void timer1_pwm_init(void) {
    DDRB |= (1 << PB5);  // Set PB5 (OC1A) as output
    TCCR1A = (1 << COM1A1) | (1 << WGM11);  // Phase-correct PWM mode
    TCCR1B = (1 << WGM13) | (1 << CS10);  // No prescaler
    ICR1 = 1023;  // Set TOP value for 10-bit PWM (1023)
    TIMSK1 |= (1 << TOIE1);  // Enable Timer1 overflow interrupt
}

// Timer1 overflow interrupt: start ADC conversion
ISR(TIMER1_OVF_vect) {
    ADCSRA |= (1 << ADSC);  // Start ADC conversion
}
