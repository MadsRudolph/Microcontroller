#include <stdio.h>
#include <util/delay.h>
#include "ssd1306.h"
#include "display.h"

// Update OLED display with PWM and limits
void update_display(uint16_t pwm_value, uint8_t min_pwm, uint8_t max_pwm, uint8_t new_vals) {
    char buffer1[20], buffer2[20], Set_Values[30];
    uint8_t display_pwm = ((uint32_t)pwm_value * 255 + 511) / 1023;  // Scale PWM to 8-bit
    uint8_t percent = ((uint32_t)pwm_value * 100 + 511) / 1023;       // Calculate PWM percentage

    // Prepare display strings
    snprintf(buffer1, sizeof(buffer1), "Duty: %3u%%     ", percent);
    snprintf(buffer2, sizeof(buffer2), "PWM:  %3u       ", display_pwm);

    // Prepare message to indicate button press for applying new values
    if (new_vals) {
        snprintf(Set_Values, sizeof(Set_Values), "Press button to set values");
    } else {
        snprintf(Set_Values, sizeof(Set_Values), "Min:%3u Max:%3u", min_pwm, max_pwm);
    }

    // Blinking bar logic (progress bar
    static uint8_t blink = 0;
    char bar[17];
    uint8_t bar_length = (percent * 16) / 100;

    for (uint8_t i = 0; i < 16; i++) {
        if (i < bar_length) {
            bar[i] = (display_pwm >= max_pwm && blink) ? ' ' : 0xFF;
        } else {
            bar[i] = ' ';
        }
    }
    bar[16] = '\0';
    blink = !blink;
    // Display the strings on the OLED
    sendStrXY(buffer2, 0, 0);
    sendStrXY(buffer1, 1, 0);
    sendStrXY(Set_Values, 6, 0);
    sendStrXY(bar, 2, 0);

    _delay_ms(10); // Small delay to allow for display update
}
