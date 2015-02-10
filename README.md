# AtTiny84-SPI-PWM  
===
 This is in progress, porting to AtTiny84 and adding functionality. Currently (pun intended), 
 
 **PWMs** are tested, mostly 
 **SPI** tested, sets PWM and reads ADC
 **ADCs** are tested, mostly   
 
**TODO: **
* ADC: Test sweep of range over SPI
* PWM: Test sweep of range over SPI
* MCU: Store values (frequency etc.) in EEPROM???
* MCU: Add watchdog timer etc?
* General: Reorganize source code and make into library/class type of thing
* ADC: Set up registers by hand, instead of using arduino analogRead()
 
AtTiny84 acts as SPI slave upon receiving a byte it sets fast PWM duty cycle to that byte. Currently the PWM frequency is hardcoded.  
 
Master SPI settings: Mode 0, MSB first, 1mHz = SPI_CLOCK_DIV16. Commands are:
    SPICOMMAND: 0b76543210
    7: getADC (0) or setPWM (1)
    6: A (0) or B (1)
    5: MSN (0) or LSN (1), N = nybble
    4: Reserved, we can fit in 10 bit numbers or more commands
    3-0: Half of a byte, this is 'data'
    I heard the arduino libraries foul up enums...
 
This uses the AtTiny85 Arduino core available on code.google.com [https://code.google.com/p/arduino-tiny/source/browse/avr/cores/tiny/pins_arduino.c?repo=core1-0150#164]  
 
    ATMEL ATTINY84 / ARDUINO
    
                               +-\/-+
                         VCC  1|    |14  GND
                 (D  0)  PB0  2|    |13  AREF (D 10)
                 (D  1)  PB1  3|    |12  PA1  (D  9) ADCA
                         PB3  4|    |11  PA2  (D  8) ADCB
            PWMA (D  2)  PB2  5|    |10  PA3  (D  7) CS
            PWMB (D  3)  PA7  6|    |9   PA4  (D  6) SCLK
            MOSI (D  4)  PA6  7|    |8   PA5  (D  5) MISO
                               +----+

