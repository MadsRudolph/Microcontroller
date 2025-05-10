#include "uart.h"
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>

volatile char uart_buffer[32];       
volatile uint8_t uart_index = 0;     
volatile uint8_t uart_rx_flag = 0;   
volatile int button_flag = 0;        

void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0A = (1 << U2X0);              
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); 
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_send(char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void uart_send_string(const char* str) {
    while (*str) uart_send(*str++);
}

ISR(USART0_RX_vect) {
    char received = UDR0;
    if (received == '\r' || received == '\n') {
        uart_buffer[uart_index] = '\0';
        uart_index = 0;
        uart_rx_flag = 1;
    } else if (uart_index < sizeof(uart_buffer) - 1) {
        uart_buffer[uart_index++] = received;
    }
}
