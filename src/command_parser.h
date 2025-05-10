#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <stdint.h> // For uint8_t, etc.

typedef enum {
    STATE_RESET,
    STATE_IDLE,
    STATE_UART_RECEIVED,
    STATE_UPDATE_DISPLAY
} SystemState;

extern uint8_t temp_min_pwm;
extern uint8_t temp_max_pwm;
extern uint8_t new_pwm_values_received;

void process_uart_command(void);

#endif
