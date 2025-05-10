#ifndef DISPLAY_H
#define DISPLAY_H
#include <stdint.h>

// Function to update the display with PWM info
void update_display(char* buffer1, char* buffer2, uint16_t buffer1_size, uint16_t buffer2_size);

#endif
