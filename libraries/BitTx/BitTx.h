// BitTx.h
//
// General purpose interrupt driven bit transmitter for ESP8266
// Message buffer consists of 16 bit words.
//	Bit 15 is level to write to pin
//	Bits 14 - 0 is tick count to maintain this level befre next change
//	Tick period is 3.2us
//  Minimum period should be > 32 ticks 96uS 
//  Maximum period is 32767 * 3.2 = 104mSec if longer required use multiple periods
// Allows ir carrirer modulation for 1 state (must use pin 2 or 14)
// rates from 10 - 400KHz with good accuracy in typical ir rates (36000)
// If rate is 0 then no modulation is used.
// Author: Bob Tidey (robert@tideys.net)

//Comment out following to use timer 0
#include <c_types.h>

//Sets up basic parameters must be called at least once
// rate is in Hz e.g. 38.5KHz = 38500. esTimer is 0 or 1
extern void bitTx_init(int pin, float rate, int esTimer);

//Checks whether tx is free to accept a new message
extern int bitTx_free();

//Basic send of message.
extern void bitTx_send(uint16* msg, int msglen, int repeats);

//Externally access to bit bang macros
extern void bitTx_bangOn();
extern void bitTx_bangOff();

//internal support
void bitTx_start();
void bitTx_stop();


