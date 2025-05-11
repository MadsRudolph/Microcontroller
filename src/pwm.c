#include <avr/io.h>
#include <avr/interrupt.h>
#include "pwm.h"

void timer1_pwm_init(void) {
    DDRB |= (1 << PB5);
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << CS10);
    ICR1 = 1023;
    TIMSK1 |= (1 << TOIE1);
}

ISR(TIMER1_OVF_vect) {
    ADCSRA |= (1 << ADSC);
}
