#ifndef UART_H
#define UART_H

#include <stdint.h>

// UART buffer and flags for handling received data
extern volatile char uart_buffer[32];  // Buffer for received UART data
extern volatile uint8_t uart_index;    // Index for the uart_buffer
extern volatile uint8_t uart_rx_flag;  // Flag to indicate a complete line of UART data received

// Function prototypes for UART communication
void uart_init(unsigned int ubrr);     // Initialize UART with given baud rate
void uart_send(char data);             // Send a single character via UART
void uart_send_string(const char* str); // Send a string via UART
void process_uart_command(void);       // Process the UART command when received

extern uint8_t temp_min_pwm, temp_max_pwm;  // Temporary PWM values received via UART
extern uint8_t new_pwm_values_received;    // Flag to indicate new PWM values received

#endif
