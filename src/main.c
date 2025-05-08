#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "I2C.h"
#include "ssd1306.h"

// === UART Setup ===
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0); // Enable RX & TX
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data
}

char uart_receive(void) {
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

void uart_send(char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void uart_send_string(const char* str) {
    while (*str) uart_send(*str++);
}

// === Timer1 PWM ===
void timer1_pwm_init() {
    DDRB |= (1 << PB5);
    TCCR1A = (1 << COM1A1) | (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS11); // Prescaler = 8
}

// === ADC Setup ===
void adc_init(void) {
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
}

uint16_t adc_read(void) {
    ADMUX = (ADMUX & 0xF0) | 0; 
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

// === UART Command Parser ===
uint8_t min_pwm = 0;
uint8_t max_pwm = 255;

void handle_uart_input(void) {
    static char buffer[20];
    static uint8_t idx = 0;

    if (UCSR0A & (1 << RXC0)) {
        char c = uart_receive();
        uart_send(c); // Echo received char for debug


        if (c == '\n') {
            buffer[idx] = '\0';

            if (strncmp(buffer, "MIN:", 4) == 0) {
                min_pwm = atoi(&buffer[4]);
                uart_send_string("Min PWM updated!\r\n");
            } else if (strncmp(buffer, "MAX:", 4) == 0) {
                max_pwm = atoi(&buffer[4]);
                uart_send_string("Max PWM updated!\r\n");
            } else {
                uart_send_string("Invalid UART command!\r\n");
            }

            idx = 0;
        } else if (idx < sizeof(buffer) - 1) {
            buffer[idx++] = c;
        }
    }
}

// === MAIN ===
int main(void) {
    timer1_pwm_init();
    I2C_Init();
    InitializeDisplay();
    adc_init();
    uart_init(MYUBRR);
    uart_send_string("UART Ready, Queen!\r\n");


    char buffer1[20];
    char buffer2[20];

    while (1) {
        handle_uart_input(); // Handle any serial commands

        uint16_t adc_value = adc_read();           // 0–1023
        uint8_t pwm_value = 255 - (adc_value / 4); // Inverted

        // Clamp PWM value
        if (pwm_value < min_pwm) pwm_value = min_pwm;
        if (pwm_value > max_pwm) pwm_value = max_pwm;

        OCR1A = pwm_value;

        uint8_t percent = (pwm_value * 100) / 255; 
        snprintf(buffer1, sizeof(buffer1), "Duty: %3u%%     ", percent); 
        snprintf(buffer2, sizeof(buffer2), "PWM:  %3u       ", pwm_value);

        // Bar Animation
        char bar[17];
        uint8_t bar_length = (percent * 16) / 100;

        for (uint8_t i = 0; i < 16; i++) {
            if (i < bar_length)
                bar[i] = 0xFF;
            else
                bar[i] = ' ';
        }
        bar[16] = '\0';

        // MAX duty
        static uint8_t blink = 0;
        char max_msg[17] = "                ";
        if (percent >= 99) {
            if (blink)
                snprintf(max_msg, sizeof(max_msg), " MAX!");
            blink = !blink;
        }

        // OLED updates
        sendStrXY(buffer2, 0, 0); // Line 0
        sendStrXY(buffer1, 1, 0); // Line 1
        sendStrXY(bar, 3, 0);     // Line 2
        sendStrXY(max_msg, 4, 0); // Line 3

        _delay_ms(200);
    }
}
