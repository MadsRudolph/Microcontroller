#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "I2C.h"
#include "ssd1306.h"

//------Nye Globale variabler + UART opsætning------//
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>

#define Baud 19200
#define MYUBRR (F_CPU / 8 / Baud - 1)  

volatile char uart_buffer[32];
volatile uint8_t uart_index = 0;
volatile uint8_t uart_rx_flag = 0;

uint8_t min_pwm = 0;
uint8_t max_pwm = 255;
//------Nye Globale variabler + UART opsætning------//

//------------------------UART-funktioner------------------------//
void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0A = (1 << U2X0);  // Double speed
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // 8-bit data
}

ISR(USART0_RX_vect) {
    char received = UDR0;

    if (received == '\n' || received == '\r') {
        uart_buffer[uart_index] = '\0';
        uart_rx_flag = 1;
        uart_index = 0;
    } else if (uart_index < sizeof(uart_buffer) - 1) {
        uart_buffer[uart_index++] = received;
    }
}
//------------------------UART-funktioner------------------------//

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
    //-------------------------test af LED-------------------------//
       /*
        Testede den ved at få den til at følge PWM-signalets duty cycle som 
        det steg eller faldt ved gradvist at tænde eller slukke LED´en for at passe til den 

        uint8_t brightness = 0;
         int8_t step = 1;
    
    while (1)
    {
        OCR1A = brightness; //sæt duty cycle (0-255)

        brightness += step; //justere lysstyrken

        //vend retning af duty cycle ved min/ max værdi
        if (brightness == 0 || brightness ==255)    //min / max grænser for Duty Cycle
        {
            step  = -step;
        }

        _delay_ms(10);
    }
    */
    //-------------------------test af LED-------------------------//
   
    //-------------------------test af OLED-Display-------------------------//
    /*uint8_t brightness = 0;
    int8_t step = 1;
    char buffer[16];

    while (1)
    {
    OCR1A = brightness;

    // Konverter til procent og formatter tekst
    uint8_t percent = (brightness * 100) / 255;
    snprintf(buffer, sizeof(buffer), "Duty: %3u%%", percent);

    // clear_display();                    // Ryd displayet            (gør at skærmen tænder og slukker ved hver opdatering: BRUG IKKE HER)                    // Ryd displayet
    sendStrXY(buffer, 0, 0);               // Vis teksten på øverste linje

    // Fortsæt fade...
    brightness += step;
    if (brightness == 0 || brightness == 255)
        step = -step;

    _delay_ms(30);
    }*/
    //-------------------------test af OLED-Display-------------------------//

    //-------------------------test af Potentiometer-------------------------//
    /*char buffer1[20];
    char buffer2[20];
    
    while (1)
    {
        uint16_t adc_value = adc_read();           // 0–1023
        uint8_t pwm_value = adc_value / 4;         // 0–255
        OCR1A = pwm_value;                         // Sæt PWM duty

        uint8_t percent = (pwm_value * 100) / 255;
        snprintf(buffer1, sizeof(buffer1), "Duty: %3u%%   ", percent);
        snprintf(buffer2, sizeof(buffer2), "PWM:  %3u     ", pwm_value);
        
        sendStrXY(buffer1, 0, 0);    //øverste linje AKA linje 1
        sendStrXY(buffer2, 1, 0);    //linje 2

        _delay_ms(50);
    }*/
    //-------------------------test af Potentiometer-------------------------//

    //-------------------------test af UART-------------------------//
    uart_init(MYUBRR);
    sei();  // enable interrupts
    //-------------------------------------------------------------//
    char buffer1[18], buffer2[18], buffer3[18], buffer4[18];
    while (1)
    {
        if (uart_rx_flag) {
            uart_rx_flag = 0;
            uint16_t min_in, max_in;
            if (sscanf((char*)uart_buffer, "%u %u", &min_in, &max_in) == 2) {
                if (min_in <= 255 && max_in <= 255 && min_in < max_in) {
                    min_pwm = min_in;
                    max_pwm = max_in;
                }
            }
        }
        uint16_t adc_value = adc_read();  // 0–1023
        uint8_t raw_pwm = adc_value / 4;  // 0–255
        // Skaler PWM til brugerens område
        uint8_t pwm_value = min_pwm + ((raw_pwm * (max_pwm - min_pwm)) / 255);
        OCR1A = pwm_value;
        uint8_t percent = (pwm_value * 100) / 255;
        //-------------------------------------------------------------//
        snprintf(buffer1, sizeof(buffer1), "PWM: %3u (%3u%%)", pwm_value, percent);
        snprintf(buffer2, sizeof(buffer2), "Min: %3u", min_pwm);
        snprintf(buffer3, sizeof(buffer3), "Max: %3u", max_pwm);
        snprintf(buffer4, sizeof(buffer4), "Raw ADC: %4u", adc_value);
        //-------------------------------------------------------------//
        sendStrXY(buffer1, 0, 0);
        sendStrXY(buffer2, 1, 0);
        sendStrXY(buffer3, 2, 0);
        sendStrXY(buffer4, 3, 0);
        _delay_ms(100);
    }
    //-------------------------test af UART-------------------------//



}