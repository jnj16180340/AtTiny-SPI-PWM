# AtTiny84-SPI-PWM  
===
 This is in progress, porting to AtTiny84 and adding functionality. Currently (pun intended), 
 
 **PWMs** are tested, kind of 
 **SPI** is untested 
 **ADCs** aren't added yet  
 
 **TODO: Port to AtTiny84 and add features**
*PWM: Investigate whether fast mode is outputting 1/255 duty cycle when set to 0x00 and change if need be
*SPI: Port it
*SPI: 2 byte input
*SPI: Commands, such as read-without-setting and change-clock-speed
*SPI: Output
*ADC: read
*Meta: Figure out a sane way to structure repos AND be able to use the arduino ide OR switch to something a little more rational  
*MCU: Store values (frequency etc.) in EEPROM???
*MCU: Add watchdog timer etc?
*MCU: *Robust* protection against activating both PWM at the same time... set option in eeprom in case we aren't using a full bridge in the future  
 
 AtTiny84 acts as SPI slave upon receiving a byte it sets fast PWM duty cycle to that byte. Currently the PWM frequency is hardcoded.  
 
 Master SPI settings: Mode 0, MSB first, 1mHz = SPI_CLOCK_DIV16. Sending BLAH 
 
 This uses the AtTiny85 Arduino core available on code.google.com [https://code.google.com/p/arduino-tiny/source/browse/avr/cores/tiny/pins_arduino.c?repo=core1-0150#164]  
 
  // ATMEL ATTINY84 / ARDUINO
  //
  //                           +-\/-+
  //                     VCC  1|    |14  GND
  //             (D  0)  PB0  2|    |13  AREF (D 10)
  //             (D  1)  PB1  3|    |12  PA1  (D  9)
  //                     PB3  4|    |11  PA2  (D  8)
  //  PWM  INT0  (D  2)  PB2  5|    |10  PA3  (D  7)
  //  PWM        (D  3)  PA7  6|    |9   PA4  (D  6)
  //  PWM        (D  4)  PA6  7|    |8   PA5  (D  5)        PWM
  //                           +----+

