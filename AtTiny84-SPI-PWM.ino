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


#include <stdlib.h>
//#include <EEPROM.h> //this would be for storing settings if we had any
#include <avr/interrupt.h>
#include <avr/io.h>        // Adds useful constants
#include <util/delay.h>    // Adds delay_ms and delay_us functions

// SPI
#define MOSI 4	//84:pp7  | 85:pp5 //somebody else switches mosi,miso pin values
#define MISO 5	//84:pp8  | 85:pp6
#define SCK  6	//84:pp9  | 85:pp7
#define CS   7	//84:pp10, PA3 | 85:pp2
//#define PCIE0  5	// Pin change interrupts //necessary???

// PWM
#define PWMA 2 //84:pp5
#define PWMB 3 //84:pp6
#define ADCA 8 //84: pp11
#define ADCB 9 //84:pp12

#define F_CPU 8000000	// This is used by delay.h library

volatile boolean csLow = false;	// chip select flag, active = low
volatile int recvByte = 0;	// incoming spi byte
volatile boolean recvFlag = false;	// have received spi byte?

// ATtiny85 outputs
//volatile uint8_t* Port[] = {&OCR0A, &OCR0B, &OCR1B};
//int Pin[] = {0, 1, 4};

void setup() {
  setupPwm();
  setupSPI(); 
}

void setupSPI(){
  // This should be complete.
  //pin setup
  pinMode(MISO, INPUT); //disabled for start condition
  pinMode(MOSI, INPUT);
  pinMode(SCK,  INPUT);
  pinMode(CS,   INPUT);

  //actual USI setup. Important... match the master settings accordingly
  USICR = 0; //set everything to zero.
  USICR = (1 << USIWM0) | (1 << USICS1) | (1 << USIOIE); //Three wire mode0, External clock positive edge both edges, Enable counter overflow interrupt

  //chip select interrupt
  // 84: Becomes PCMSK1 + PCMSK0 (xxxx11,pcint10,9,8 + pcint7,...,0)
  // 84: So pp10 is still PCINT3... woot
  PCMSK0 |= (1<<PCINT3); //PB3 Pin change interrupt PCINT1, which is handled by ISR for PCINT0_vect
  //PCMSK |= 0b00001000;
  GIMSK  |= (1<<PCIE0); // 84: PCIE1 is also available.
  //GIMSK |= 0b00100000;
  sei();
}

void setupPwm(){
  // PWM setup
  // NB OC1x provides 16 bit pwm, but is used for the USI...
  pinMode(PWMA, OUTPUT);
  pinMode(PWMB, OUTPUT);
  // Configure counter/timer1 for fast PWM on PB4
  // COM1B0: 2 -> normal, 3 -> inverting
  //85: Enable PWMB | Set COM1(B1,B0) to (1,0) for OC1x active, ~OC1x not connected
  //GTCCR = 1<<PWM1B | 2<<COM1B0; //84: GTCCR does less/different stuff??
  
  // set normal mode for OC0A,B
  TCCR0A = 1<<COM0A1 | 1<<COM0B1 | 1<<WGM01 | 1<<WGM00; //WGM01,WGM00
  // TCCR1: : CS02,01,00 (0,0,1)=fclk to (1,0,1) =fclk/1024, (1,1,0) = external clk
  TCCR1A = 1<<CS00; //set 1<<WGM02 for 'mode 7' fast pwm, top less than 0xFF see  p83//CS02,01,00 are clock select bits, see p84
  //TCCR1 = 3<<COM1A0 | 7<<CS10;
  // CS10 is a clock rate divisor; COM1A0 set above is bugfix which is irrelevant (we're not using inverted mode, which needs COM1A0 to be nonzero)
  // but scope (crappy computer scope) says 62500ish...
  // 1: 40000
  // 2: 20000
  // 3: 8000
  // 4: 4000
  // 7: 500
  // 8: 250
  //TCCR1 = 1<<CS10; // sets PWM frequency to about 40kHz??? if we believe the scope, see 85:p89
  
  SetPwm(0);
  
}

// Sets PWM on pin 4
inline void SetPwm (int duty) {
  OCR0A = duty;
  OCR0B = duty;
}

void loop() {
  if(recvFlag){
    SetPwm(recvByte);
    recvFlag = false;
  }
  
  //test code delete me
  for(int i = 0; i<0xFF; i++){
   //It seems to get reeaal wonky if both PWM pins draw too much current
   //But not always!!!
   OCR0A = i;
   OCR0B = 0xFF-i;
   //SetPwm(i);
   delay(20); 
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
