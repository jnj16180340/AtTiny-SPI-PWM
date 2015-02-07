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
  
  setValue(pwmControl);
  pwmControl+=0b00000001;
  //setValue(0b00001000);
  delay(20);
  
}
