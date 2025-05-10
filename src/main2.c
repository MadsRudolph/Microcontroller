#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>

// Include custom modules
#include "I2C.h"
#include "ssd1306.h"     // OLED display driver
#include "command_parser.h"  // Command parser functions
#include "button.h"          // Button interrupt and handling
#include "display.h"         // Display update functions
#include "uart.h"           // UART functions
#include "pwm.h"            // PWM functions

// === UART Setup ===
#define BAUD 19200
#define MYUBRR F_CPU/8/BAUD-1  // Calculate UART Baud Rate Register value

// UART variables
volatile char uart_buffer[32];       // Buffer to store incoming UART data
volatile uint8_t uart_index = 0;     // Index to keep track of position in buffer
volatile uint8_t uart_rx_flag = 0;   // Flag set when a command is fully received

// === PWM Variables ===
volatile uint16_t pwm_value = 0; // Stores current PWM value
uint8_t min_pwm = 0;
uint8_t max_pwm = 255;
uint8_t temp_min_pwm = 0;
uint8_t temp_max_pwm = 255;
uint8_t new_pwm_values_received = 0;

// === Timer1 PWM ===
void timer1_pwm_init() {
    DDRB |= (1 << PB5); // Set PB5 (OC1A) as output

    TCCR1A = (1 << COM1A1) | (1 << WGM11);  // PWM mode, phase-correct
    TCCR1B = (1 << WGM13) | (1 << CS10);    // No prescaler, WGM13 = 1
    ICR1 = 1023;                            //TOP = 1023 (Match med 10-bit ADC)
    TIMSK1 |= (1 << TOIE1);                 // Enable overflow interrupt
}

// === ADC with interrupt ===
void adc_init(void) {
    ADMUX = (1 << REFS0); // Reference voltage = AVcc
    ADCSRA = (1 << ADEN)  // Enable ADC
           | (1 << ADIE)  // Enable ADC interrupt
           | (1 << ADATE) // Auto trigger enable
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler = 128 = 125 KHz
}

// ADC conversion complete interrupt
ISR(ADC_vect) {
    uint16_t adc_value = ADC; // Read 10-bit ADC value

    uint16_t scaled_min = ((uint32_t)min_pwm * 1023 + 127) / 255; // Scale min PWM to 10-bit
    uint16_t scaled_max = ((uint32_t)max_pwm * 1023 + 127) / 255; // Scale max PWM to 10-bit

    // Clamp value between min and max PWM
    if (adc_value < scaled_min) adc_value = scaled_min; // Set to min if below
    if (adc_value > scaled_max) adc_value = scaled_max; // Set to max if above

    OCR1A = adc_value; // Update PWM output
    pwm_value = adc_value; // Store current ADC value for display
}

// Interrupt Service Routine for UART receive
ISR(USART0_RX_vect) {
    char received = UDR0;
    if (received == '\r' || received == '\n') {
        uart_buffer[uart_index] = '\0'; // Null-terminate the string
        uart_index = 0;
        uart_rx_flag = 1; // Signal that full command is received
    } else if (uart_index < sizeof(uart_buffer) - 1) {
        uart_buffer[uart_index++] = received; // Store character
    }
}

// === Button Interrupt for resetting the system ===
ISR(INT4_vect) {
    button_flag = 1; // Set button flag when interrupt occurs
}

int main(void) {
    // Initialize peripherals
    timer1_pwm_init();
    I2C_Init();
    InitializeDisplay();
    adc_init();
    uart_init(MYUBRR);   // Initialize UART
    button_init();       // Initialize button interrupt
    sei();               // Enable global interrupts

    uart_send_string("UART Ready, input values for MIN: / MAX: !\r\n");

    SystemState current_state = STATE_RESET;
    char buffer1[20]; // Display line 1
    char buffer2[20]; // Display line 2

    while (1) {
        switch (current_state) {

            // Wait for button to reset system
            case STATE_RESET:
                if (button_flag) {
                    _delay_ms(100); // Debounce
                    button_flag = 0;

                    // Apply new PWM values if received
                    if (new_pwm_values_received) {
                        min_pwm = temp_min_pwm;
                        max_pwm = temp_max_pwm;
                        new_pwm_values_received = 0;
                        ADCSRA |= (1 << ADSC); // Force ADC conversion with new values
                        uart_send_string("MIN/MAX updated via button press.\r\n");
                    }

                    InitializeDisplay(); // Re-initialize OLED
                    clear_display();
                    current_state = STATE_IDLE;
                }
                break;

            // Idle state: check for UART input or go to update display
            case STATE_IDLE:
                if (uart_rx_flag) {
                    current_state = STATE_UART_RECEIVED;
                } else {
                    current_state = STATE_UPDATE_DISPLAY;
                }
                break;

            // UART data received: process command
            case STATE_UART_RECEIVED:
                uart_rx_flag = 0;
                process_uart_command(); // Process the UART command
                current_state = STATE_IDLE;
                break;

            // Update OLED display with PWM info
            case STATE_UPDATE_DISPLAY:
                update_display(buffer1, buffer2); // Call display update function
                _delay_ms(20); // Small delay for display update

                // Check for button or new UART input
                if (button_flag) {
                    _delay_ms(100);
                    current_state = STATE_RESET;
                } else if (uart_rx_flag) {
                    current_state = STATE_UART_RECEIVED;
                } else {
                    current_state = STATE_IDLE;
                }

                break;
        }
    }
}
