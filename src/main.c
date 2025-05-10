// Include necessary libraries
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include "I2C.h"
#include "ssd1306.h"  // OLED display driver

// === UART Setup ===
#define BAUD 19200
#define MYUBRR F_CPU/8/BAUD-1  // Calculate UART Baud Rate Register value

// UART variables
volatile char uart_buffer[32];       // Buffer to store incoming UART data
volatile uint8_t uart_index = 0;     // Index to keep track of position in buffer
volatile uint8_t uart_rx_flag = 0;   // Flag set when a command is fully received
volatile int button_flag = 0;        // Flag for button press interrupt

// Initialize UART with given baud rate
void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8); // Set baud rate high byte
    UBRR0L = (unsigned char)ubrr;        // Set baud rate low byte
    UCSR0A = (1 << U2X0);                // Double speed mode
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); // Enable Rx, Tx, and interrupt
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data

    PORTE |= (1<<PE4);  // Enable pull-up resistor on PE4 (button)
    EIMSK |= (1<<INT4); // Enable external interrupt INT4
    EICRB |= (1<<ISC41); // Trigger INT4 on rising edge
}

// Send a single character over UART
void uart_send(char data) {
    while (!(UCSR0A & (1 << UDRE0))); // Wait until buffer is empty
    UDR0 = data;                      // Send data
}

// Send a null-terminated string over UART
void uart_send_string(const char* str) {
    while (*str) uart_send(*str++);
}

// Interrupt Service Routine for UART receive
ISR(USART0_RX_vect) {
    char received = UDR0;
    if (received == '\r' || received == '\n') {
        uart_buffer[uart_index] = '\0'; // Null-terminate the string
        uart_index = 0;
        uart_rx_flag = 1;               // Signal that full command is received
    } else if (uart_index < sizeof(uart_buffer) - 1) {
        uart_buffer[uart_index++] = received; // Store character
    }
}

// === Timer1 PWM ===
void timer1_pwm_init() {
    DDRB |= (1 << PB5); // Set PB5 (OC1A) as output

    TCCR1A = (1 << COM1A1) | (1 << WGM11);  // PWM mode, phase-correct
    TCCR1B = (1 << WGM13) | (1 << CS10);    // No prescaler, WGM13 = 1
    ICR1 = 1023;                            //TOP = 1023 (Match med 10-bit ADC) Sample-rate = 7819 Hz (skal ændres hvis sample-rate skal ændres)

    TIMSK1 |= (1 << TOIE1);                 //aktivere overflow interrupt
}

// === ADC konvertering ved overflow ===
ISR(TIMER1_OVF_vect) {
    ADCSRA |= (1 << ADSC);  // Start ADC-konvertering når overlow sker
}

// === ADC with interrupt ===
volatile uint16_t pwm_value = 0; // Stores current PWM value

void adc_init(void) {
    ADMUX = (1 << REFS0); // Reference voltage = AVcc
    ADCSRA = (1 << ADEN)  // Enable ADC
           | (1 << ADIE)  // Enable ADC interrupt
           | (1 << ADATE) // Auto trigger enable
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler = 128 = 125 KHz


}

// PWM limit values
uint8_t min_pwm = 0;
uint8_t max_pwm = 255;

// Temporary variables for new settings (to be confirmed with button)
uint8_t temp_min_pwm = 0;
uint8_t temp_max_pwm = 255;
uint8_t new_pwm_values_received = 0;

// ADC conversion complete interrupt
ISR(ADC_vect) {
    uint16_t adc_value = ADC;       // Read 10-bit ADC value

    uint16_t scaled_min = ((uint32_t)min_pwm * 1023 + 127) / 255; // Scale min PWM to 10-bit
    uint16_t scaled_max = ((uint32_t)max_pwm * 1023 + 127) / 255; // Scale max PWM to 10-bit


    // Clamp value between min and max PWM
    if (adc_value < scaled_min) adc_value = scaled_min; // Set to min if below
    if (adc_value > scaled_max) adc_value = scaled_max; // Set to max if above

    OCR1A = adc_value; // Update PWM output
    pwm_value = adc_value; // Store current ADC value for display
    
}

// === UART Command Parser ===
void process_uart_command(void) {
    uart_send_string("Got: ");
    uart_send_string((char*)uart_buffer);
    uart_send_string("\r\n");

    // Check for MIN or MAX command and store value temporarily
    if (strncmp((char*)uart_buffer, "MIN:", 4) == 0) {
        uint8_t value = atoi((char*)&uart_buffer[4]);
        if (value >= temp_max_pwm) {
            uart_send_string("Error: MIN value cannot be greater than MAX!\r\n");
        } else {
            temp_min_pwm = value;
            uart_send_string("Temp MIN stored. Press button to apply.\r\n");
            new_pwm_values_received = 1;
        }
    } 
    
    else if (strncmp((char*)uart_buffer, "MAX:", 4) == 0) {
        uint8_t value = atoi((char*)&uart_buffer[4]);
        if  (value <= temp_min_pwm) {
            uart_send_string("Error: MAX value cannot be less than MIN!\r\n");
        } else {
            temp_max_pwm = value;
            uart_send_string("Temp MAX stored. Press button to apply.\r\n");
            new_pwm_values_received = 1;
        }
    } 
    else {
        uart_send_string("Invalid UART command! Use MIN: or MAX:\r\n");
    }
}

// === FSM States for Main Control Loop ===
typedef enum {
    STATE_RESET,
    STATE_IDLE,
    STATE_UART_RECEIVED,
    STATE_UPDATE_DISPLAY
} SystemState;

// Button interrupt to flag user confirmation
ISR(INT4_vect) {
    button_flag = 1;
}

int main(void) {
    // Initialize peripherals
    timer1_pwm_init();
    I2C_Init();
    InitializeDisplay();
    adc_init();
    uart_init(MYUBRR);
    clear_display();
    sei(); // Enable global interrupts

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
                        ADCSRA |= (1 << ADSC); // force ADC conversion with new values
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
                process_uart_command();
                current_state = STATE_IDLE;
                break;

            // Update OLED display with PWM info
            case STATE_UPDATE_DISPLAY: {
                uint8_t display_pwm = ((uint32_t)pwm_value * 255 + 511) /1023; // Scale to 8-bit for display
                uint8_t percent = ((uint32_t)pwm_value * 100 + 511) / 1023; // Calculate percentage

                snprintf(buffer1, sizeof(buffer1), "Duty: %3u%%     ", percent);
                snprintf(buffer2, sizeof(buffer2), "PWM:  %3u       ", display_pwm);

                char Set_Values[20];
                if (new_pwm_values_received) {
                    snprintf(Set_Values, sizeof(Set_Values), "Waiting for button");
                } else {
                    snprintf(Set_Values, sizeof(Set_Values), "Min:%3u Max:%3u", min_pwm, max_pwm);
                }

                // Create simple bar graph to visualize PWM level
                char bar[17];
                uint8_t bar_length = (percent * 16) / 100;
                for (uint8_t i = 0; i < 16; i++) {
                    bar[i] = (i < bar_length) ? 0xFF : ' ';
                }
                bar[16] = '\0';

                // Blink "MAX!" warning when PWM is at max
                static uint8_t blink = 0;
                char max_msg[17] = "                ";
                if (display_pwm >= max_pwm) {
                    if (blink) snprintf(max_msg, sizeof(max_msg), " MAX!");
                    blink = !blink;
                }

                // Display all messages on screen
                sendStrXY(buffer2, 0, 0);
                sendStrXY(buffer1, 1, 0);
                sendStrXY(Set_Values, 6, 0);
                sendStrXY(bar, 2, 0);
                sendStrXY(max_msg, 4, 0);

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
}