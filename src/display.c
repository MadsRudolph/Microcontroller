#include "display.h"
#include <stdio.h>
#include "ssd1306.h"
#include <stdint.h>
#include "pwm.h"

void update_display(char* buffer1, char* buffer2, uint16_t buffer1_size, uint16_t buffer2_size) {
    uint8_t display_pwm = ((uint32_t)pwm_value * 255 + 511) / 1023;
    uint8_t percent = ((uint32_t)pwm_value * 100 + 511) / 1023;

    snprintf(buffer1, buffer1_size, "Duty: %3u%%     ", percent);
    snprintf(buffer2, buffer2_size, "PWM:  %3u       ", display_pwm);

    sendStrXY(buffer2, 0, 0);
    sendStrXY(buffer1, 1, 0);
    sendStrXY("Min:   Max:   ", 6, 0);  
    sendStrXY("PWM Level", 2, 0);
}
