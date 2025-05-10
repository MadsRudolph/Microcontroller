#ifndef PWM_H
#define PWM_H
#include <stdint.h>

void pwm_init(void);
extern volatile uint16_t pwm_value;
extern uint8_t min_pwm;
extern uint8_t max_pwm;

#endif
