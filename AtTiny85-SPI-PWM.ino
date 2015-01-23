#include <stdlib.h>
//#include <EEPROM.h>
#include <avr/interrupt.h>
#include <avr/io.h>        // Adds useful constants
#include <util/delay.h>    // Adds delay_ms and delay_us functions

// SPI
#define MOSI 0	//physical pin 5
#define MISO 1	//physical pin 6
#define SCK  2	//physical pin 7
#define CS   3	//physical pin 2

#define PCIE0  5	// Pin change interrupts

#define F_CPU 8000000	// This is used by delay.h library

volatile boolean csLow = false;	// chip select flag, active = low
volatile int recvByte = 0;	// incoming spi byte
volatile boolean recvFlag = false;	// have received spi byte?

// ATtiny85 outputs
//volatile uint8_t* Port[] = {&OCR0A, &OCR0B, &OCR1B};
//int Pin[] = {0, 1, 4};

void setup() {
  
  // PWM setup
  pinMode(4, OUTPUT);
  pinMode(3,INPUT); // CS for spi
 
  // Configure counter/timer1 for fast PWM on PB4
  // COM1B0: 2 -> normal, 3 -> inverting
  GTCCR = 1<<PWM1B | 2<<COM1B0;
  //TCCR1 = 3<<COM1A0 | 7<<CS10;
  // CS10 is a clock rate divisor; COM1A0 set above is bugfix which is irrelevant (we're not using inverted mode, which needs COM1A0 to be nonzero)
  // but scope (crappy computer scope) says 62500ish...
  // 1: 40000
  // 2: 20000
  // 3: 8000
  // 4: 4000
  // 7: 500
  // 8: 250
  TCCR1 = 2<<CS10; // sets PWM frequency to about 20kHz
  
  SetPwm(0);
  
  setupSPI(); 
}

void setupSPI(){
  //pin setup
  pinMode(MISO, INPUT); //disabled for start condition
  pinMode(MOSI, INPUT);
  pinMode(SCK,  INPUT);
  pinMode(CS,   INPUT);

  //actual USI setup. Important... match the master settings accordingly
  USICR = 0; //set everything to zero.
  USICR = (1 << USIWM0) | (1 << USICS1) | (1 << USIOIE); //Three wire mode0, External clock positive edge both edges, Enable interrupt

  //chip select interrupt
  PCMSK |= (1<<PCINT3); //PB3 Pin change interrupt PCINT1, which is handled by ISR for PCINT0_vect
  //PCMSK |= 0b00001000;
  GIMSK  |= (1<<PCIE0);
  //GIMSK |= 0b00100000;
  sei();
}

// Sets PWM on pin 4
inline void SetPwm (int duty) {
  OCR1B = duty;
}

void loop() {
  if(recvFlag){
    SetPwm(recvByte);
    recvFlag = false;
  }
}

ISR(PCINT0_vect){//readies the system for a SPI transaction if the pin is low
  if(digitalRead(CS)==LOW){
    //SetPwm(255);
    csLow = true; // //resetting is the job of the SPI overflow handler   
  }
  else{//when chip is deselected
    csLow = false;
    //SetPwm(0);
  }
}

//then Master sends the message which should trigger the ISR, oversimplified part
//the expectation is that Master sends one byte and asks for one byte in return
ISR(USI_OVF_vect){//overflow
  byte c = USIDR; //read the USI Data Buffer
  USISR = 1 << USIOIF; //clear the interrupt flag, it needs to be set to 1 to clear and reset the counter
  if(csLow){  //this slave is selected, so byte is for this slave
    pinMode(MISO, OUTPUT); //get ready to send out a byte
	USIDR = recvByte; //load the outgoing byte, in this case the previous byte
	if(!recvFlag){
		recvFlag=true;//next byte will be zero as master is expecting a value and sends junk to read it
		recvByte= c; //save the byte
	}
	else{//outgoing byte has been clocked out, reverse the MISO pin
		pinMode(MISO,INPUT);
	}
    csLow = false;
  }
  else{
    pinMode(MISO,INPUT); //reverse it so it doesn't interfere with other slaves
  } 
}  // end of interrupt service routine
