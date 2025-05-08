#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include "I2C.h"
#include "ssd1306.h"

// === UART Setup ===
#define BAUD 19200
#define MYUBRR F_CPU/8/BAUD-1

volatile char uart_buffer[32];
volatile uint8_t uart_index = 0;
volatile uint8_t uart_rx_flag = 0;
volatile int button_flag = 0; // Flag for button press

void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0A = (1 << U2X0);
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);


    PORTE |= (1<<PE4);  // Aktivér pull-up modstand
    EIMSK |= (1<<INT4); // Aktiver INT4
    EICRB |= (1<<ISC41); // Trigger INT4 på stigende flanke 
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

// === Timer1 PWM ===
void timer1_pwm_init() {
    DDRB |= (1 << PB5);
    TCCR1A = (1 << COM1A1) | (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS11);
}

// === ADC ===
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

// === UART Commands ===
uint8_t min_pwm = 0;
uint8_t max_pwm = 255;

void process_uart_command(void) {
    uart_send_string("Got: ");
    uart_send_string((char*)uart_buffer);
    uart_send_string("\r\n");

    if (strncmp((char*)uart_buffer, "MIN:", 4) == 0) {
        min_pwm = atoi((char*)&uart_buffer[4]);
        uart_send_string("Min PWM updated!\r\n");
    } else if (strncmp((char*)uart_buffer, "MAX:", 4) == 0) {
        max_pwm = atoi((char*)&uart_buffer[4]);
        uart_send_string("Max PWM updated!\r\n");
    } else {
        uart_send_string("Invalid UART command!\r\n");
    }
}

// === FSM States ===
typedef enum {
    STATE_RESET,
    STATE_IDLE,
    STATE_UART_RECEIVED,
    STATE_UPDATE_DISPLAY
} SystemState;

// Interrupt Service Routine (ISR) for INT4 (knaptryk)
ISR(INT4_vect) {
    button_flag = 1; // Sæt flag når knappen trykkes
}

int main(void) {
    timer1_pwm_init();
    I2C_Init();
    InitializeDisplay();
    adc_init();
    uart_init(MYUBRR);
    clear_display();
    sei();

    uart_send_string("UART Ready, input values for MIN and MAX!\r\n");

    SystemState current_state = STATE_RESET;
    char buffer1[20];
    char buffer2[20];

    while (1) {

      

        switch (current_state) {

            case STATE_RESET:
                if (button_flag) {
                    _delay_ms(100);
                    button_flag = 0; // Reset button flag
                  
                InitializeDisplay();
                clear_display();
                current_state = STATE_IDLE; }
                break;

            case STATE_IDLE:
                if (uart_rx_flag) {
                    current_state = STATE_UART_RECEIVED;
                } else {
                    current_state = STATE_UPDATE_DISPLAY;
                }
                break;

            case STATE_UART_RECEIVED:
                uart_rx_flag = 0;
                process_uart_command();
                current_state = STATE_IDLE;
                break;

            case STATE_UPDATE_DISPLAY: {
                uint16_t adc_value = adc_read();
                uint8_t pwm_value = 255 - (adc_value / 4);

                if (pwm_value < min_pwm) pwm_value = min_pwm;
                if (pwm_value > max_pwm) pwm_value = max_pwm;

                OCR1A = pwm_value;

                uint8_t percent = (pwm_value * 100) / 255;
                snprintf(buffer1, sizeof(buffer1), "Duty: %3u%%     ", percent);
                snprintf(buffer2, sizeof(buffer2), "PWM:  %3u       ", pwm_value);

                char bar[17];
                uint8_t bar_length = (percent * 16) / 100;
                for (uint8_t i = 0; i < 16; i++) {
                    bar[i] = (i < bar_length) ? 0xFF : ' ';
                }
                bar[16] = '\0';

                static uint8_t blink = 0;
                char max_msg[17] = "                ";
                if (percent >= 99) {
                    if (blink) snprintf(max_msg, sizeof(max_msg), " MAX!");
                    blink = !blink;
                }

                sendStrXY(buffer2, 0, 0);
                sendStrXY(buffer1, 1, 0);
                sendStrXY(bar, 3, 0);
                sendStrXY(max_msg, 4, 0);

                _delay_ms(200);
                  if (button_flag) {
                    _delay_ms(100);
                       
                    current_state = STATE_RESET; // Go back to reset state
                  }
                  else if (uart_rx_flag) {
                    current_state = STATE_UART_RECEIVED;
                   } else
                current_state = STATE_IDLE;

                break;
            }
        }
    }
}