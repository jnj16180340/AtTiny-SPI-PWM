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

// TODO this is chip specific
// SPI
#define MOSI 4	//84:pp7  | 85:pp5 //somebody else switches mosi,miso pin values
#define MISO 5	//84:pp8  | 85:pp6
#define SCK  6	//84:pp9  | 85:pp7
#define CS   7	//84:pp10, PA3 | 85:pp2
//#define PCIE0  5	// Pin change interrupts //necessary???

//TODO this is chip specific
// PWM
#define PWMA 2 //84:pp5
#define PWMB 3 //84:pp6
#define ADCA A1 //84: pp11
#define ADCB A2 //84:pp12
#define PROTECT_BRIDGE true // If we set PWM on 1 channel, disable it on the other

#define F_CPU 8000000	// This is used by delay.h library???

// These are the bits for SPI commands
//#define SPIGETADC (0) or setPWM (1)
// 6: A (0) or B (1)
// 5: MSN (0) or LSN (1), N = nybble
// 4: Reserved, we can fit in 10 bit numbers or more commands
// 3-0: Half of a byte, this is 'data'

void setupPwm();
void setupAdc();
inline void setPwm(int);
inline void parseInput(char);

volatile boolean csLow = false;	// chip select flag, active = low
volatile char recvByte = 0x00;	// incoming spi byte
volatile char outByte = 0x00; // in case we need an extra buffer... like if recvByte gets overwritten before the command can get parsed, which SHOULDN'T happen...
volatile boolean recvFlag = false;	// have received spi byte?
volatile char PWMA_STATE = 0x00; // these will mostly be used for dealing with commands
volatile char PWMB_STATE = 0x00;
volatile char ADCA_STATE = 0x00;
volatile char ADCB_STATE = 0x00;

void setupSPI(){
  // This should be complete.
  //pin setup
  pinMode(MISO, INPUT); //disabled for start condition
  pinMode(MOSI, INPUT);
  pinMode(SCK,  INPUT);
  pinMode(CS,   INPUT);
  
  //TODO this is chip specific
  //TODO AND it's not as simple as just changing the register names :(
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

void setupADC(){
  pinMode(ADCA, INPUT);
  pinMode(ADCB, INPUT);
}

void setupPwm(){
  // PWM setup
  // NB OC1x provides 16 bit pwm, but is used for the USI...
  //TODO this is chip specific
  //TODO AND it's not as simple as just changing the register names :(
  pinMode(PWMA, OUTPUT);
  pinMode(PWMB, OUTPUT);
  // Configure counter/timer1 for fast PWM on PB4
  // COM1B0: 2 -> normal, 3 -> inverting
  //85: Enable PWMB | Set COM1(B1,B0) to (1,0) for OC1x active, ~OC1x not connected
  //GTCCR = 1<<PWM1B | 2<<COM1B0; //84: GTCCR does less/different stuff??
  
  // set normal mode for OC0A,B
  TCCR0A = 1<<COM0A1 | 1<<COM0B1 | 1<<WGM01 | 1<<WGM00 | 1<<COM0A0 | 1<<COM0B0; //set COM0*0 for inverted mode, so that we can turn completely off.
  // TCCR1: : CS02,01,00 (0,0,1)=fclk to (1,0,1) =fclk/1024, (1,1,0) = external clk
  TCCR1A = 1<<CS00; //set 1<<WGM02 for 'mode 7' fast pwm, top less than 0xFF see  p83//CS02,01,00 are clock select bits, see p84. CS00 is fioclk undivided
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
  
  OCR0A = 0xFF; //Set both PWM to 0
  OCR0B = 0xFF;
  
}

inline void setPwm () {
  // Call this to set PWMs, after figuring out what they should be
  OCR0A = PWMA_STATE;
  OCR0B = PWMB_STATE;
  //_delay_us(10);
}

inline void getAdc(){
  // read this to update both adc states 
  // read takes 100 usecs, don't think it needs a manual delay
  // I have no idea whether it's getting the msb or lsb of analogRead, which returns 0...1023
  ADCA_STATE = analogRead(ADCA);
  _delay_us(100);
  ADCB_STATE = analogRead(ADCB);
  _delay_us(100);
}

//TODO this is chip specific
inline void parseInput(char incoming){
  // incoming is because i don't know of calling it recvByte would redefine the global recvByte
  // I think this maybe should go in the ISR??
  // Do we send out SPI byte immediately on receiving CS?????
  // I think so...
  // So, Master will need to send a command, then send a garbage command
  // and look at the reply from that... the top half should be the same
  
  // Take an SPI byte (command) and do stuff
  // SPICOMMAND: 0b76543210
  // 7: getADC (0) or setPWM (1)
  // 6: A (0) or B (1)
  // 5: MSN (0) or LSN (1), N = nybble
  // 4: Reserved, we can fit in 10 bit numbers or more commands
  // 3-0: Half of a byte, this is 'data'
  // I heard the arduino libraries foul up enums...
  outByte = recvByte & (0b11110000); // the least 4 bits are data
  if(incoming & (1<<7)){
    // set pwm 
    if(incoming & (1<<6)){
      // channel a
      PWMA_STATE = OCR0A; // necessary? probs no
      if(incoming & (1<<5)){
	// MSN
	// Set PWMA: MSN
	if(PROTECT_BRIDGE){OCR0B = 0xFF;} //Turn OFF PWMB
	OCR0A = (PWMA_STATE & 0b00001111) + (incoming << 4);
	outByte += (PWMA_STATE>>4);
	
      }
      else{
	// LSN
	// Set PWMA: LSN
	if(PROTECT_BRIDGE){OCR0B = 0xFF;} //Turn OFF PWMB
	OCR0A = (PWMA_STATE & 0b11110000) + (incoming & 0b00001111);
	outByte += (PWMA_STATE & 0b00001111);
      }
      
    }
    else{
      // channel b
      PWMB_STATE = OCR0B; // necessary? probs no
      if(incoming & (1<<5)){
	// MSN
	// Set PWMB: MSN
	if(PROTECT_BRIDGE){OCR0A = 0xFF;} //Turn OFF PWMA
	OCR0B = (PWMB_STATE & 0b00001111) + (incoming << 4);
	PWMB_STATE = OCR0B;
	outByte += (PWMB_STATE>>4);
      }
      else{
	// LSN
	// Set PWMB: LSN
	if(PROTECT_BRIDGE){OCR0A = 0xFF;} //Turn OFF PWMA
	OCR0B = (PWMB_STATE & 0b11110000) + (incoming & 0b00001111);
	PWMB_STATE = OCR0B;
	outByte += (PWMB_STATE & 0b00001111);
      }
    }
  }
  else{
    // get adc
    // IDK but it just seems mode 'solid' than an else statement
    // though i doubt that's true 
    if(incoming & (1<<6)){
      // channel a
      ADCA_STATE = analogRead(ADCA); _delay_us(100); //necessary?
      if(incoming & (1<<5)){
	// MSN
	// Get ADCA: MSN
	outByte += ADCA_STATE>>4; //copy the highest 4 bits
      }
      else{
	// LSN
	// Get ADCA: LSN
	outByte += ADCA_STATE & 0b00001111; //copy the lowest 4 bits
      }  
    }
    else{
      // channel b
      ADCB_STATE = analogRead(ADCB); _delay_us(100); //necessary?
      if(incoming & (1<<5)){
	// MSN
	// Get ADCB: MSN
	outByte += ADCB_STATE>>4; //copy the highest 4 bits
      }
      else{
	// LSN
	// Get ADCB: LSN
	outByte += ADCB_STATE & 0b00001111;
      }
    }
  }  
  recvByte = outByte; // recvByte will be transmitted next timer we're selected
}

void setup() {
  setupADC();
  setupPwm();
  setupSPI(); 
}

void loop() {
  if(recvFlag){
    parseInput(recvByte);
    recvFlag = false;
  }
}

ISR(PCINT0_vect){//readies the system for a SPI transaction if the pin is low
  if(digitalRead(CS)==LOW){
    csLow = true; // //resetting is the job of the SPI overflow handler   
  }
  else{//when chip is deselected
    csLow = false;
  }
}

//then Master sends the message which should trigger the ISR, oversimplified part
//the expectation is that Master sends one byte and asks for one byte in return
ISR(USI_OVF_vect){//overflow, when we gots 8 bits
  byte c = USIDR; //read the USI Data Buffer
  USISR = 1 << USIOIF; //clear the interrupt flag, it needs to be set to 1 to clear and reset the counter
  if(csLow){  //this slave is selected, so byte is for this slave
    pinMode(MISO, OUTPUT); //get ready to send out a byte
	USIDR = recvByte; //load the outgoing byte, in this case the PREVIOUS byte
	if(!recvFlag){
		recvFlag=true;//next byte will be zero as master is expecting a value and sends junk to read it
		recvByte= c; //save the byte, which was already copied out of USIDR
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
