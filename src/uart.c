#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include "uart.h"

// UART buffer and flags for handling received data
volatile char uart_buffer[32];
volatile uint8_t uart_index = 0;
volatile uint8_t uart_rx_flag = 0;

uint8_t temp_min_pwm = 0;    // Temporary min PWM value
uint8_t temp_max_pwm = 255;  // Temporary max PWM value
uint8_t new_pwm_values_received = 0; // Flag for new PWM values

// Initialize UART with a given baud rate
void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0A = (1 << U2X0);  // Enable double speed mode
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);  // Enable RX, TX, and RX complete interrupt
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // Set data frame format to 8-bit data
    
    // Setup button interrupt (PE4)
    PORTE |= (1<<PE4);  // Enable pull-up resistor on PE4
    EIMSK |= (1<<INT4); // Enable external interrupt INT4
    EICRB |= (1<<ISC41); // Trigger interrupt on rising edge
}

// Send a single character via UART
void uart_send(char data) {
    while (!(UCSR0A & (1 << UDRE0))); // Wait until the UART is ready to transmit
    UDR0 = data;  // Transmit the character
}

// Send a string of characters via UART
void uart_send_string(const char* str) {
    while (*str) uart_send(*str++); // Send each character until null-terminator
}

// Interrupt service routine for UART RX
ISR(USART0_RX_vect) {
    char received = UDR0;  // Read received character
    if (received == '\r' || received == '\n') {
        uart_buffer[uart_index] = '\0'; // Null-terminate string when newline or carriage return
        uart_index = 0;
        uart_rx_flag = 1; // Set flag to indicate full line received
    } else if (uart_index < sizeof(uart_buffer) - 1) {
        uart_buffer[uart_index++] = received; // Store character in buffer
    }
}

// Process UART command to update PWM values
void process_uart_command(void) {
    uart_send_string("Got: ");
    uart_send_string((char*)uart_buffer);  // Echo back received string
    uart_send_string("\r\n");

    // Process MIN: command
    if (strncmp((char*)uart_buffer, "MIN:", 4) == 0) {
        int value = atoi((char*)&uart_buffer[4]); // Convert string to integer
        if (value < 0 || value > 255) {
            uart_send_string("Error: MIN must be between 0 and 255!\r\n");
        } else if (value >= temp_max_pwm) {
            uart_send_string("Error: MIN cannot be >= MAX!\r\n");
        } else {
            temp_min_pwm = value;  // Update temporary MIN value
            uart_send_string("Temp MIN stored. Press button to apply.\r\n");
            new_pwm_values_received = 1;  // Flag new values
        }
    } 
    // Process MAX: command
    else if (strncmp((char*)uart_buffer, "MAX:", 4) == 0) {
        int value = atoi((char*)&uart_buffer[4]); // Convert string to integer
        if (value < 0 || value > 255) {
            uart_send_string("Error: MAX must be between 0 and 255!\r\n");
        } else if (value <= temp_min_pwm) {
            uart_send_string("Error: MAX cannot be <= MIN!\r\n");
        } else {
            temp_max_pwm = value;  // Update temporary MAX value
            uart_send_string("Temp MAX stored. Press button to apply.\r\n");
            new_pwm_values_received = 1;  // Flag new values
        }
    } else {
        uart_send_string("Invalid UART command! Use MIN: or MAX:\r\n");
    }
}
