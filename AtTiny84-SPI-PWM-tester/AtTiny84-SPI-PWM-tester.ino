/*
Test a SPI slave
*/

/*
#define SELECT LOW
#define DESELECT HIGH
*/

#include "SPI.h" // necessary library
int ss=10; // using digital pin 10 for SPI slave select
byte pwmControl = 0b00000000;

#define MOSI 11
#define MISO 12
#define SCLK 13
#define CS 10
 
void setup()
{
  pinMode(ss, OUTPUT); // we use this for SS pin
  pinMode(3,OUTPUT);
  SPI.setDataMode(SPI_MODE0); 
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  // Necessary for un-noisy comms with ATTiny85 8mHz
  SPI.setBitOrder(MSBFIRST); 
  SPI.begin(); // wake up the SPI bus.
}
 
void setValue(byte value)
{
  digitalWrite(ss, LOW);
  delay(1); //necessary
  SPI.transfer(value); // send value (0~255)
  delay(1); //maybe not necessary
  digitalWrite(ss, HIGH);
  delay(1); //maybe not necessary
}

void loop()
{
  //setValue(0b00000000);
  //delay(500);
  //setValue(0b11111111);
  //delay(500);
  
  
  pwmControl= 0xFF;
  setValue(pwmControl);
  delay(2000);
  pwmControl = 0x00;
  setValue(pwmControl);
  delay(2000);
  
}
