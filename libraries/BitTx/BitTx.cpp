// BitTx.cpp
//
// General purpose interrupt driven bit transmitter for ESP8266
// Message buffer consists of 16 bit words.
//	Bit 15 is level to write to pin
//	Bits 14 - 0 is tick count to maintain this level befre next change
//	Tick period is 3.2us
//  Minimum period should be > 32 ticks 100uS 
//  Maximum period is 32767 * 3.2 = 104mSec if longer required use multiple periods
// Allows ir carrirer modulation for 1 state (must use pin 2 or 14)
// rates from 10 - 400KHz with good accuracy in typical ir rates (36000)
// If rate is 0 then no modulation is used.
// Author: Bob Tidey (robert@tideys.net)

#include "BitTx.h"
#include <arduino.h>
#include <i2s_reg.h>
#include <i2s.h>
#include <string.h>

//Define macros for local use of On/Off 
#define BANG_ON  ESP8266_REG(bangAddrOn) = bangDataOn; 
#define BANG_OFF ESP8266_REG(bangAddrOff) = bangDataOff;

//Base i2s word clock = 16,000,000 / 34
#define BASE_I2S_WCLOCK 4705882.35

static uint32_t bangAddrOn, bangAddrOff, bangDataOn, bangDataOff;

extern "C" {
void rom_i2c_writeReg_Mask(int, int, int, int, int, int);
}

static int txPin = 14;

//Transmit mode variables
static int tx_msg_active = 0; //set 1 to activate message sending
static uint16 nextPeriod = 0;
static uint16* tx_bufstart; // the message buffer pointer start
static uint16* tx_bufptr; // the message buffer pointer during transmission
static int tx_bitCount;
static int tx_bitCounter;
static int tx_repeats = 12; // Number of repeats to send
static int tx_repeat = 0; //counter for repeats
static int txTimer = 0; // timer to use

static unsigned long espNext = 0; //Holds interrupt next count for timer0

void ICACHE_RAM_ATTR bitTxIsr() {
	if (*tx_bufptr & 0x8000)
		BANG_ON
	else
		BANG_OFF
	
	//Set next period
	nextPeriod = *tx_bufptr & 0x7fff;
	//terminate isr if a period is 0 or short (32uSec)
	if(nextPeriod < 10) bitTx_stop();
	
	tx_bufptr++;
	tx_bitCounter--;
	if(tx_bitCounter <= 0) {
		tx_repeat--;
		if(tx_repeat <= 0) {
			bitTx_stop();
			tx_msg_active = 0;
		} else {
			tx_bufptr = tx_bufstart;
			tx_bitCount = tx_bitCounter;
		}
	}
	if(tx_msg_active) {
		if(txTimer == 0 ) {
			espNext += nextPeriod<<8;
			timer0_write(espNext);
		} else {
			timer1_write(nextPeriod);
			timer1_enable(TIM_DIV265, TIM_EDGE, TIM_SINGLE);
		}
	}
}

/**
  Check for send free
**/
int bitTx_free() {
  return !tx_msg_active;
}

/**
  Send a message
**/
void bitTx_send(uint16 *msg, int msglen, int repeats) {
  tx_bufstart = msg;
  tx_bufptr = msg;
  tx_bitCount = msglen;
  tx_bitCounter = msglen;
  tx_repeats = repeats;
  tx_repeat = repeats;
  bitTx_start();
  tx_msg_active = 1;
}

/**
  Set things up to transmit messages
**/
void bitTx_init(int pin, float rate, int esTimer) {
	int i,j,div1,div2;
	float target, diff, diffMin;

	txPin = pin;
	txTimer = esTimer;
	digitalWrite(txPin, 0);
	pinMode(txPin, OUTPUT);
	if (rate == 0) {
		bangAddrOn = 0x304;
		bangAddrOff = 0x308;
		bangDataOn   = 1<<txPin;
		bangDataOff = bangDataOn;
	} else {
		target = BASE_I2S_WCLOCK / rate;
		diffMin = target;
		for(i = 1; i < 64; i++) {
			j = round(target/i);
			diff = abs(target - i*j);
			if(j < 64 && diff < diffMin) {
				diffMin = diff;
				div1 = i;
				div2 = j;
			}
		}
		//Serial.printf("div1 %d div2 %d\r\n",div1,div2);
		I2SC &= 0xf0000cff;
		I2SC |= div1 << I2SBD | div2 << I2SCD | 1 << I2SBM;
		//Set up address and start i2s clock to get word clock
		if(txPin == 2) {
			I2SC |= 0x100; //0x2<<8 start rx clock
			bangAddrOn = 0x838;
		} else {
			txPin = 14; // force to 14 if not 2 for modulation
			I2SC |= 0x200; //0x1<<8 start tx clock
			bangAddrOn = 0x80C;
		}
		bangAddrOff = bangAddrOn;
		bangDataOff = ESP8266_REG(bangAddrOn) & 0xfffffe0f; //base function reg
		bangDataOn = bangDataOff | 0x00000010; //function 1 i2s ws
		bangDataOff = bangDataOff | 0x00000030; //function 3 GPIO
	}
}

void bitTx_start() {
	noInterrupts();
	rom_i2c_writeReg_Mask(i2c_bbpll, i2c_bbpll_hostid, i2c_bbpll_en_audio_clock_out, i2c_bbpll_en_audio_clock_out_msb, i2c_bbpll_en_audio_clock_out_lsb, 1);
	if(txTimer == 0) {
		timer0_isr_init();
		timer0_attachInterrupt(bitTxIsr);
		espNext = ESP.getCycleCount()+10000;
		timer0_write(espNext);
	} else {
		timer1_isr_init();
		timer1_attachInterrupt(bitTxIsr);
		timer1_enable(TIM_DIV265, TIM_EDGE, TIM_SINGLE);
		timer1_write(1000);
	}
	interrupts();
}

void ICACHE_RAM_ATTR  bitTx_stop() {
	if(txTimer == 0) {
		timer0_detachInterrupt();
	} else {
		timer1_disable();
		timer1_detachInterrupt();
	}
	rom_i2c_writeReg_Mask(i2c_bbpll, i2c_bbpll_hostid, i2c_bbpll_en_audio_clock_out, i2c_bbpll_en_audio_clock_out_msb, i2c_bbpll_en_audio_clock_out_lsb, 0);
	pinMode(txPin, INPUT);
}


//Externally callable bit bang on
void bitTx_bangOn() {
	BANG_ON
}

//Externally callable bit bang off
void bitTx_bangOff() {
	BANG_OFF
}

