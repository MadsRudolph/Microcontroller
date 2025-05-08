#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "I2C.h"
#include "ssd1306.h"

void timer1_pwm_init()
{
    // Sæt PB5 (Pin 11) som output
    DDRB |= (1 << PB5);

    // Fast PWM, 8-bit: WGM13:0 = 0b0101
    TCCR1A = (1 << COM1A1) | (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS11); // Prescaler = 8

}


// ADC initialisering
void adc_init(void)
{
    ADMUX = (1 << REFS0);                    // AVCC reference, kanal 0
    ADCSRA = (1 << ADEN)                     // Enable ADC
           | (1 << ADPS2) | (1 << ADPS1);    // Prescaler = 64
}

// ADC læsning
uint16_t adc_read(void)
{
    ADMUX = (ADMUX & 0xF0) | 0;              // Kanal 0 (A0 / PF0)
    ADCSRA |= (1 << ADSC);                   // Start ADC
    while (ADCSRA & (1 << ADSC));            // Vent på færdig
    return ADC;
}


int main(void)
{
    timer1_pwm_init();
    I2C_Init();
    InitializeDisplay();
    adc_init();

  
    char buffer1[20];
    char buffer2[20];
    
    while (1)
    {
        uint16_t adc_value = adc_read();           // 0–1023
        uint8_t pwm_value = 255 - (adc_value / 4); // 0–255

        OCR1A = pwm_value;                         // Set PWM duty

        uint8_t percent = (pwm_value * 100) / 255; 
        snprintf(buffer1, sizeof(buffer1), "Duty: %3u%%   ", percent); 
        snprintf(buffer2, sizeof(buffer2), "PWM:  %3u     ", pwm_value);

        // ✨ Bar Animation
        char bar[17];
        uint8_t bar_length = (percent * 16) / 100;

        for (uint8_t i = 0; i < 16; i++) {
            if (i < bar_length)
                bar[i] = 0xFF; // Full block (or use '=' if OLED can't handle 0xFF)
            else
                bar[i] = ' ';
        }
        bar[16] = '\0';

        // 💅 MAX duty = blinking sass
        static uint8_t blink = 0;
        char max_msg[17] = "                ";

        if (percent >= 99) {
            if (blink)
                snprintf(max_msg, sizeof(max_msg), " MAX!");
            blink = !blink;
        }
        sendStrXY(buffer2, 0, 0); // Line 0 
        sendStrXY(buffer1, 1, 0); // Line 1
        sendStrXY(bar, 2, 0);     // Line 2
        sendStrXY(max_msg, 3, 0); // Line 3 

        _delay_ms(200); // Give it a bit more breathing room for da drama 💅
    }
}
