# AtTiny85-SPI-PWM
/*
 
 AtTiny85 acts as SPI slave (Mode 0); upon receiving a byte it sets fast PWM duty cycle to that byte. Currently the PWM frequency is hardcoded.
 
 This uses the AtTiny85 Arduino core available on code.google.com
 
 TODO:
   Store values (frequency etc.) in EEPROM
   Multiplex SPI pins to free up another pin for more PWM???? NO, it SUCKS.
   Multiplex MISO pin for analog read?? POSSIBLE.
   SPI:
     Multi bytes OR less than 8 bit resolution
     Clock prescaler and wider range of frequencies
     H-bridge control... Is it possible??? NO.
 
 ATtiny85 test code
 
 The connections to the ATTiny are as follows:
 ATTiny    Arduino    Info
 Pin  1  - 5          RESET / Rx (Not receiving any data)
 Pin  2  - 3          CS (chip select fot spi)
 Pin  3  - 4          PWM output
 Pin  4  -            GND
 Pin  5  - 0          MOSI
 Pin  6  - 1          MISO
 Pin  7  - 2 / A1     SCK
 Pin  8  -   +Vcc
 
 SEE:
 http://www.technoblogy.com/show?LE0
 http://www.technoblogy.com/show?KVO

 */
