#include "command_parser.h"
#include "uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
void process_uart_command(void) {
    uart_send_string("Got: ");
    uart_send_string((char*)uart_buffer);
    uart_send_string("\r\n");

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
