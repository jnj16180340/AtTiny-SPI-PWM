/*
Test a SPI slave
Now with serial io for extra debugging efficiency!!!!!
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

#define SERIALSPEED 9600
 
void setup()
{
  Serial.begin(SERIALSPEED);
  Serial.println("start");
  
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
  char returnedByte = 0x00;
  digitalWrite(ss, LOW);
  delay(1); //necessary
  returnedByte = SPI.transfer(value); // send value (0~255)
  delay(1); //maybe not necessary
  digitalWrite(ss, HIGH);
  delay(1); //maybe not necessary
  Serial.println("returned:");
  Serial.println(returnedByte&(0b00001111)); //just print the data
}

void loop()
{
  // 7: getADC (0) or setPWM (1)
  // 6: A (0) or B (1)
  // 5: MSN (0) or LSN (1), N = nybble
  // 4: Reserved, we can fit in 10 bit numbers or more commands
  // 3-0: Half of a byte, this is 'data'
  
  if(0){ //test MSB of PWM A,B
  setValue(0b11110000);
  delay(1000);
  setValue(0b10110000);
  delay(1000);
  setValue(0b11111111); //1111. inverted, is off
  setValue(0b10111111);
  delay(1000);
  }
  
  if(0){ //test LSB of PWM A,B
  setValue(0b11010000);
  delay(1000);
  setValue(0b10010000);
  delay(1000);
  setValue(0b11011111); //1111. inverted, is off
  setValue(0b10011111);
  delay(1000);
  }
  
  if(0){ //test MSB of ADC A,B
    //NB needs to go through 2 cycles to return measurement
  setValue(0b01110000);
  delay(1000);
  setValue(0b00110000);
  delay(1000);
  }
  
  if(1){ //test LSB of ADC A,B
    //NB needs to go through 2 cycles to return measurement
  setValue(0b01010000);
  delay(1000);
  setValue(0b00010000);
  delay(1000);
  }
  
  
  
  Serial.println("loop");
}
