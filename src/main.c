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

    PORTE |= (1<<PE4);  // Enable pull-up resistor
    EIMSK |= (1<<INT4); // Enable INT4
    EICRB |= (1<<ISC41); // Trigger INT4 on rising edge 
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

// === ADC with interrupt ===
volatile uint8_t pwm_value = 0;

void adc_init(void) {
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN)
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0)
           | (1 << ADIE)
           | (1 << ADATE);
    ADCSRB = 0x00;
    ADCSRA |= (1 << ADSC);
}

uint8_t min_pwm = 0;
uint8_t max_pwm = 255;

// New temp variables for pending values
uint8_t temp_min_pwm = 0;
uint8_t temp_max_pwm = 255;
uint8_t new_pwm_values_received = 0;

ISR(ADC_vect) {
    uint16_t adc_value = ADC;
    uint8_t temp_pwm = adc_value / 4;

    if (temp_pwm < min_pwm) temp_pwm = min_pwm;
    if (temp_pwm > max_pwm) temp_pwm = max_pwm;

    pwm_value = temp_pwm;
    OCR1A = pwm_value;
}

// === UART Commands ===
void process_uart_command(void) {
    uart_send_string("Got: ");
    uart_send_string((char*)uart_buffer);
    uart_send_string("\r\n");

    if (strncmp((char*)uart_buffer, "MIN:", 4) == 0) {
        temp_min_pwm = atoi((char*)&uart_buffer[4]);
        uart_send_string("Temp MIN stored. Press button to apply.\r\n");
        new_pwm_values_received = 1;
    } else if (strncmp((char*)uart_buffer, "MAX:", 4) == 0) {
        temp_max_pwm = atoi((char*)&uart_buffer[4]);
        uart_send_string("Temp MAX stored. Press button to apply.\r\n");
        new_pwm_values_received = 1;
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

ISR(INT4_vect) {
    button_flag = 1;
}

int main(void) {
    timer1_pwm_init();
    I2C_Init();
    InitializeDisplay();
    adc_init();
    uart_init(MYUBRR);
    clear_display();
    sei();

    uart_send_string("UART Ready, input values for MIN: / MAX: !\r\n");

    SystemState current_state = STATE_RESET;
    char buffer1[20];
    char buffer2[20];

    while (1) {
        switch (current_state) {

            case STATE_RESET:
                if (button_flag) {
                    _delay_ms(100);
                    button_flag = 0;

                    if (new_pwm_values_received) {
                        min_pwm = temp_min_pwm;
                        max_pwm = temp_max_pwm;
                        new_pwm_values_received = 0;

                        uart_send_string("MIN/MAX updated via button press.\r\n");
                    }

                    InitializeDisplay();
                    clear_display();
                    current_state = STATE_IDLE;
                }
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
                uint8_t local_pwm = pwm_value;

                OCR1A = local_pwm;
                uint8_t percent = (local_pwm * 100) / 255;

                snprintf(buffer1, sizeof(buffer1), "Duty: %3u%%     ", percent);
                snprintf(buffer2, sizeof(buffer2), "PWM:  %3u       ", local_pwm);

                char Set_Values[20];
                if (new_pwm_values_received) {
                    snprintf(Set_Values, sizeof(Set_Values), "Waiting for button");
                } else {
                    snprintf(Set_Values, sizeof(Set_Values), "Min:%3u Max:%3u", min_pwm, max_pwm);
                }

                char bar[17];
                uint8_t bar_length = (percent * 16) / 100;
                for (uint8_t i = 0; i < 16; i++) {
                    bar[i] = (i < bar_length) ? 0xFF : ' ';
                }
                bar[16] = '\0';

                static uint8_t blink = 0;
                char max_msg[17] = "                ";
                if (local_pwm >= max_pwm) {
                if (blink) snprintf(max_msg, sizeof(max_msg), " MAX!");
                blink = !blink;
}


                sendStrXY(buffer2, 0, 0);
                sendStrXY(buffer1, 1, 0);
                sendStrXY(Set_Values, 6, 0);
                sendStrXY(bar, 2, 0);
                sendStrXY(max_msg, 4, 0);

                _delay_ms(200);

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
