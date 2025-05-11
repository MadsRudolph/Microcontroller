#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

void update_display(uint16_t pwm_value, uint8_t min_pwm, uint8_t max_pwm, uint8_t new_vals);

#endif
