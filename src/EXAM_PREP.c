/*
 * Exam Notes – Embedded C Programming on ATmega2560
 * Name: [MADS RUDOLPH]
 * Date: [15.5.2024]
 * Description: Answers and examples for the 6 core exam questions
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ===================== 1. Port Interfacing =====================
/*
 * Explain how to configure microcontroller ports as input/output.
 * Include example of reading and writing to pins using DDRx, PORTx, and PINx.
 * Code from opg. 5 or a mock example.
 */
void port_example() {
    DDRB |= (1 << PB7);       // PB7 as output
    DDRD &= ~(1 << PD5);      // PD5 as input
    PORTD |= (1 << PD5);      // Enable pull-up on PD5

    if ((PIND & (1 << PD5)) == 0) {
        PORTB |= (1 << PB7);  // Turn on LED if button pressed
    } else {
        PORTB &= ~(1 << PB7); // Turn off LED
    }
}

// ===================== 2. Serial Interfaces (UART/I2C) =====================
/*
 * Describe UART, SPI, I2C. Include differences, speed, sync/async.
 * Show UART initialization and data transmission from opg. 5.
 */

void uart_init_example(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0A = (1 << U2X0); // Double speed
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_send_example(char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

// ===================== 3. Interrupts vs Polling =====================
/*
 * Explain the difference between interrupts and polling.
 * Pros/cons, types of interrupts (external, timer, ADC, UART, etc.)
 * Give ISR examples (e.g., UART RX or external INT4).
 */

ISR(USART0_RX_vect) {
    // UART receive interrupt example
    char received = UDR0;
    // Handle received char
}

ISR(INT4_vect) {
    // External interrupt on PE4
}

// ===================== 4. Timers =====================
/*
 * What is a timer? What can it be used for?
 * Example from opg. 5: Timer1 used for PWM generation + ADC trigger.
 */

void timer1_pwm_init_example() {
    DDRB |= (1 << PB5); // OC1A as output
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << CS10);
    ICR1 = 1023;
    TIMSK1 |= (1 << TOIE1); // Enable overflow interrupt
}

ISR(TIMER1_OVF_vect) {
    ADCSRA |= (1 << ADSC); // Start ADC conversion
}

// ===================== 5. Microcontroller Architecture =====================
/*
 * Describe MCU architecture: Registers, SRAM, Flash, EEPROM, stack.
 * What is the stack, how is it used? How is memory addressed?
 * Example: local variables, function calls (use the stack).
 */

void some_function() {
    uint8_t local_var = 42; // Stored on the stack
    (void)local_var; // Just to suppress unused warning
}

// ===================== 6. C Structures & FSM =====================
/*
 * Control structures (if, switch, while), data structures (enums, structs).
 * Functions, modularity, FSM explanation with opg. 5's state machine.
 */

typedef enum {
    STATE_IDLE,
    STATE_RECEIVE,
    STATE_APPLY
} State;

void fsm_example() {
    State current = STATE_IDLE;

    switch (current) {
        case STATE_IDLE:
            // do idle stuff
            break;
        case STATE_RECEIVE:
            // handle UART
            break;
        case STATE_APPLY:
            // update PWM
            break;
    }
}

// ===================== Main =====================
int main(void) {
    // You can leave this blank or use it to test the above examples
    while (1) {
    }
}
