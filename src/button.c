#include "button.h"
#include <avr/io.h>
#include <avr/interrupt.h>

int button_flag = 0;

void button_init(void) {
    PORTE |= (1<<PE4);  // Enable pull-up resistor on PE4 (button)
    EIMSK |= (1<<INT4); // Enable external interrupt INT4
    EICRB |= (1<<ISC41); // Trigger INT4 on rising edge
}

ISR(INT4_vect) {
    button_flag = 1;
}
