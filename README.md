# AVR PWM Control with UART and OLED Display

This project demonstrates an AVR microcontroller-based system that controls PWM output based on analog input, with real-time configuration via UART commands and visual feedback using an OLED display over I2C. It uses Timer1 for PWM output, ADC for input sensing, and UART for serial communication. The system also includes a physical push-button to confirm changes, and displays current status using an SSD1306 OLED screen.

Features include PWM output via Timer1 on pin PB5 (OC1A), ADC input to dynamically control PWM duty cycle, and an SSD1306 OLED display that shows current PWM value, duty cycle, a visual bar graph, and MIN/MAX settings. Users can send UART commands like `MIN:<value>` and `MAX:<value>` to update limits. New values are stored temporarily and only applied after a button press, adding a safety mechanism against accidental changes. The logic is structured using a simple finite state machine (FSM).

An example OLED screen layout might look like this:
```
PWM:  128       
Duty:  50%      
████████        
Min:  20 Max:200
```

**Hardware Requirements:** ATmega328P or similar 16MHz AVR MCU, SSD1306 OLED display (128x64) via I2C, a push-button connected to INT4 (e.g., PE4 with pull-up), USB-UART adapter or serial monitor, and a sensor/potentiometer on ADC0.

**Software Requirements:** AVR-GCC toolchain, AVRDUDE for flashing, and a compatible programmer (like USBasp or Arduino as ISP). Required libraries include `I2C.h` and `ssd1306.h`, which handle I2C communication and OLED rendering.

**UART Command Examples:**
```
MIN:20   # Temporarily set minimum PWM value
MAX:200  # Temporarily set maximum PWM value
```
Important: You must press the physical button after sending these commands to apply the changes.

**File Structure:**
```
/project-root
├── main.c           # Main application with FSM and all logic
├── I2C.h/.c         # I2C communication utilities
├── ssd1306.h/.c     # OLED display driver
└── README.md        # Project documentation
```

**To Build and Flash the Firmware:**
```
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -o main.elf main.c I2C.c ssd1306.c
avr-objcopy -O ihex main.elf main.hex
avrdude -c usbasp -p m328p -U flash:w:main.hex
```

How it works: The ADC continuously samples the analog signal and scales it to a PWM duty cycle between 0–255. This duty cycle is applied to Timer1’s output pin. UART input is handled via interrupts, buffering each command until a newline. When a command like "MIN:50" is received, it’s stored temporarily. Pressing the button triggers an interrupt that sets those values permanently. The OLED display updates continuously with the PWM value, a percentage bar graph, and either “Min/Max” or a “Waiting for button” message if new values are pending. If the PWM reaches the MAX value, a blinking "MAX!" warning is shown.

This project is licensed under the MIT License.

Pull requests are welcome. For significant changes, please open an issue first to discuss what you'd like to change.

For questions or feedback, please open an issue or reach out via GitHub Discussions.
