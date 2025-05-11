// === Included Libraries ===
#include <avr/io.h>         // Core I/O register definitions
#include <util/delay.h>     // Delay functions
#include <stdio.h>          // sprintf & friends
#include <stdlib.h>         // atoi, etc.
#include <string.h>         // String functions
#include <avr/interrupt.h>  // Interrupt macros
#include "I2C.h"            // I2C driver
#include "ssd1306.h"        // OLED display driver

// === UART Setup ===
#define BAUD 19200
#define MYUBRR ((F_CPU/8/BAUD)-1)  // UART baud rate formula with double speed

// UART Buffers & Flags
volatile char uart_buffer[32];   // Buffer for incoming UART text
volatile uint8_t uart_index = 0; // Index in the buffer
volatile uint8_t uart_rx_flag = 0; // Set when full line is received
volatile int button_flag = 0;    // Set on button press (INT4)

// === UART Initialization ===
void uart_init(unsigned int ubrr) {
    // Set baud rate
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;

    UCSR0A = (1 << U2X0); // Enable double speed mode
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); // Enable RX, TX, and RX complete interrupt
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data
    
    // Setup button interrupt
    PORTE |= (1<<PE4);  // Enable pull-up resistor on PE4 (button)
    EIMSK |= (1<<INT4); // Enable external interrupt INT4
    EICRB |= (1<<ISC41); // Trigger INT4 on rising edge
}

// Send a single character over UART
void uart_send(char data) {
    while (!(UCSR0A & (1 << UDRE0))); // Wait until ready
    UDR0 = data; // Send char
}

void uart_send_string(const char* str) {
    while (*str) uart_send(*str++); // Send string
}

//interrupt service routine for UART RX
// === UART RX Interrupt ===
ISR(USART0_RX_vect) {
    char received = UDR0;
    if (received == '\r' || received == '\n') {
        uart_buffer[uart_index] = '\0'; // End string
        uart_index = 0;
        uart_rx_flag = 1;
    } else if (uart_index < sizeof(uart_buffer) - 1) {
        uart_buffer[uart_index++] = received;
    }
}

// === PWM Using Timer1 ===
void timer1_pwm_init() {
    DDRB |= (1 << PB5); // Set PB5 (OC1A) as output

    // Phase-correct PWM, no prescaler
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << CS10);
    ICR1 = 1023; // TOP = 1023 (matches 10-bit ADC)
    TIMSK1 |= (1 << TOIE1); // Enable Timer1 overflow interrupt
}

// === Timer Overflow Interrupt Starts ADC ===
ISR(TIMER1_OVF_vect) {
    ADCSRA |= (1 << ADSC); // Start ADC conversion
}

// === ADC Init & ISR ===
volatile uint16_t pwm_value = 0; // For display use

void adc_init(void) {
    ADMUX = (1 << REFS0); // AVcc as reference voltage, ADC0 as input
    ADCSRA = (1 << ADEN)  | // Enable ADC
             (1 << ADIE)  | // Enable ADC interrupt
             (1 << ADATE) | // Auto trigger
             (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128 → 125kHz
}

// === PWM Clamp Range ===
uint8_t min_pwm = 0;
uint8_t max_pwm = 255;

// Temporary variables for new settings (to be confirmed with button)
uint8_t temp_min_pwm = 0;
uint8_t temp_max_pwm = 255;
uint8_t new_pwm_values_received = 0;

// === ADC Conversion Complete ISR ===
ISR(ADC_vect) {
    uint16_t adc_value = ADC; // Read ADC (0–1023)

    // Scale min/max from 8-bit to 10-bit
    uint16_t scaled_min = ((uint32_t)min_pwm * 1023 + 127) / 255;
    uint16_t scaled_max = ((uint32_t)max_pwm * 1023 + 127) / 255;

    // Clamp ADC value
    if (adc_value < scaled_min) adc_value = scaled_min; //Set to min if below
    if (adc_value > scaled_max) adc_value = scaled_max; //Set to max if above

    OCR1A = adc_value;       // Update PWM duty
    pwm_value = adc_value;   // Store for OLED
}

// === UART Command Parser ===
void process_uart_command(void) {
    uart_send_string("Got: ");
    uart_send_string((char*)uart_buffer);
    uart_send_string("\r\n");

    if (strncmp((char*)uart_buffer, "MIN:", 4) == 0) {
        int value = atoi((char*)&uart_buffer[4]);
        if (value < 0 || value > 255) {
            uart_send_string("Error: MIN must be between 0 and 255!\r\n");
        } else if (value >= temp_max_pwm) {
            uart_send_string("Error: MIN cannot be >= MAX!\r\n");
        } else {
            temp_min_pwm = value;
            uart_send_string("Temp MIN stored. Press button to apply.\r\n");
            new_pwm_values_received = 1;
        }
    } 
    else if (strncmp((char*)uart_buffer, "MAX:", 4) == 0) {
        int value = atoi((char*)&uart_buffer[4]);
        if (value < 0 || value > 255) {
            uart_send_string("Error: MAX must be between 0 and 255!\r\n");
        } else if (value <= temp_min_pwm) {
            uart_send_string("Error: MAX cannot be <= MIN!\r\n");
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


// === FSM States ===
typedef enum {
    STATE_APPLY,
    STATE_IDLE,
    STATE_UART_RECEIVED,
    STATE_UPDATE_DISPLAY
} SystemState;

// === Button Interrupt ===
ISR(INT4_vect) {
    button_flag = 1;
}

// === Main Function ===
int main(void) {
    // Initialize peripherals
    timer1_pwm_init();
    I2C_Init();
    InitializeDisplay();
    adc_init();
    uart_init(MYUBRR);
    clear_display();
    sei(); // Enable global interrupts

    uart_send_string("Input values for minimun or max\r\n");
    uart_send_string("Format: MIN:<value> or MAX:<value> (0 to 255)\r\n");
    uart_send_string("Example: MIN:50 or MAX:200\r\n");


    SystemState current_state = STATE_UPDATE_DISPLAY; // Start in display update state
    char buffer1[20], buffer2[20]; // Buffers for display strings

    while (1) {
        switch (current_state) {
            case STATE_APPLY:
                // Wait for button to confirm changes
                if (button_flag) {
                    _delay_ms(100);
                    button_flag = 0;

                    if (new_pwm_values_received) {
                        min_pwm = temp_min_pwm;
                        max_pwm = temp_max_pwm;
                        new_pwm_values_received = 0;
                        ADCSRA |= (1 << ADSC); // Trigger ADC to apply new limits
                        uart_send_string("MIN/MAX updated via button press.\r\n");
                    }

                    InitializeDisplay();
                    clear_display();
                    current_state = STATE_IDLE;
                }
                break;

            case STATE_IDLE:
                // Check for UART input or move to update
                current_state = uart_rx_flag ? STATE_UART_RECEIVED : STATE_UPDATE_DISPLAY;
                break;

            case STATE_UART_RECEIVED:
                uart_rx_flag = 0;
                process_uart_command();
                current_state = STATE_IDLE;
                break;

            case STATE_UPDATE_DISPLAY: {
                // Convert 10-bit PWM to 8-bit and percentage
                uint8_t display_pwm = ((uint32_t)pwm_value * 255 + 511) / 1023;
                uint8_t percent = ((uint32_t)pwm_value * 100 + 511) / 1023;

                // Format strings
                snprintf(buffer1, sizeof(buffer1), "Duty: %3u%%     ", percent);
                snprintf(buffer2, sizeof(buffer2), "PWM:  %3u       ", display_pwm);

                char Set_Values[30];
                if (new_pwm_values_received) {
                    snprintf(Set_Values, sizeof(Set_Values), "Press button to set values");
                } else {
                    snprintf(Set_Values, sizeof(Set_Values), "Min:%3u Max:%3u", min_pwm, max_pwm);
                }

                // Blinking bar logic
                static uint8_t blink = 0;
                char bar[17];
                uint8_t bar_length = (percent * 16) / 100;

                for (uint8_t i = 0; i < 16; i++) {
                    if (i < bar_length) {
                        // Blink bar if at max
                        if (display_pwm >= max_pwm) {
                            bar[i] = blink ? 0xFF : ' ';
                        } else {
                            bar[i] = 0xFF;
                        }
                    } else {
                        bar[i] = ' ';
                    }
                }
                bar[16] = '\0';
                blink = !blink; // Toggle for next frame

                // Send text to OLED
                sendStrXY(buffer2, 0, 0);
                sendStrXY(buffer1, 1, 0);
                sendStrXY(Set_Values, 6, 0);
                sendStrXY(bar, 2, 0);

                _delay_ms(10); // Frame delay

                // State transitions
                if (button_flag) {
                    _delay_ms(100);
                    current_state = STATE_APPLY;
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
