#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "uart.h"
#include "pwm.h"
#include "adc.h"
#include "display.h"
#include "I2C.h"
#include "ssd1306.h"

// FSM states
typedef enum {
    STATE_APPLY,
    STATE_IDLE,
    STATE_UART_RECEIVED,
    STATE_UPDATE_DISPLAY
} SystemState;

// Button flag (INT4)
volatile int button_flag = 0;

// ISR for button press
ISR(INT4_vect) {
    button_flag = 1;
}

int main(void) {
    // === Init Section ===
    timer1_pwm_init();
    I2C_Init();
    InitializeDisplay();
    adc_init();
    uart_init(((F_CPU / 8 / 19200) - 1)); // BAUD = 19200
    clear_display();
    sei(); // Enable global interrupts

    uart_send_string("Input values for minimun or max\r\n");
    uart_send_string("Format: MIN:<value> or MAX:<value> (0 to 255)\r\n");
    uart_send_string("Example: MIN:50 or MAX:200\r\n");

    // FSM Start State
    SystemState current_state = STATE_UPDATE_DISPLAY;

    while (1) {
        switch (current_state) {
            case STATE_APPLY:
                if (button_flag) {
                    _delay_ms(100);
                    button_flag = 0;

                    if (new_pwm_values_received) {
                        min_pwm = temp_min_pwm;
                        max_pwm = temp_max_pwm;
                        new_pwm_values_received = 0;
                        ADCSRA |= (1 << ADSC);
                        uart_send_string("MIN/MAX updated via button press.\r\n");
                    }

                    InitializeDisplay();
                    clear_display();
                    current_state = STATE_IDLE;
                }
                break;

            case STATE_IDLE:
                current_state = uart_rx_flag ? STATE_UART_RECEIVED : STATE_UPDATE_DISPLAY;
                break;

            case STATE_UART_RECEIVED:
                uart_rx_flag = 0;
                process_uart_command();
                current_state = STATE_IDLE;
                break;

            case STATE_UPDATE_DISPLAY:
                update_display(pwm_value, min_pwm, max_pwm, new_pwm_values_received);

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