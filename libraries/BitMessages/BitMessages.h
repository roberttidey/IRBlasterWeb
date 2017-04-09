// BitMessages.h
//
// Defines Messages and pulse periods to use with BitTx bit bang library
// msg format string is csv with elements h:period,l:period,1:bbb(binary),4:hhh(hex)
// data bits from 1: or 4: are represented from a period array 00001111 where up to 4 pulses are allowed for 0 and 4 for a 1
// if a period is 0 then it is ignored.
// Author: Bob Tidey (robert@tideys.net)
#include <c_types.h>

#define TICK_PERIOD 3.2
#define HEX_ESCAPE '%'
#define COUNT_ESCAPE '#'

//bit definitions for special handling when adding data pulses
#define SPECIAL_NORMAL 0x0000
#define SPECIAL_RC6 0x0001

struct deviceData {
	char* deviceName;
	char* header;
	char* trailer;
	char* pulses0;
	char* pulses1;
	int frequency;
	int special;
	int repeatDelay; //mSec
	int bitCount;
	int minRepeat; //repeat 1 is send once
	char** buttons;
};

// make message pulse buffer routines
extern int bitMessages_addPulses(uint16* msg, char* pulseString, int factor);
extern int bitMessages_addDataPulses(uint16* msg, char* dataString, int bitCount, char* pulses0, char* pulses1, int special);
extern int bitMessages_getDevice(char* deviceString);
extern int bitMessages_getButton(int device, char* buttonString);
extern int bitMessages_addDelay(uint16* msg, int delay);
extern int bitMessages_makeMsg(uint16* msg, char* header, char* trailer, char* dataString, int bitCount, char* pulses0, char* pulses1, int special, int repeatDelay);
extern int bitMessages_makeNamedMsg(uint16* msg, char* deviceString, char* buttonString, int repeat, int bits);

//DeviceData access routines
extern int bitMessages_getDeviceCount();
extern char* bitMessages_getDeviceDataDeviceName(int device);
extern char* bitMessages_getDeviceDataHeader(int device);
extern char* bitMessages_getDeviceDataTrailer(int device);
extern char* bitMessages_getDeviceDataPulses0(int device);
extern char* bitMessages_getDeviceDataPulses1(int device);
extern char* bitMessages_getDeviceDataButton(int device, int button);
extern int bitMessages_getDeviceFrequency(int device);
extern int bitMessages_getDeviceSpecial(int device);
extern int bitMessages_getDeviceBitCount(int device);
extern int bitMessages_getDeviceRepeatDelay(int device);
extern int bitMessages_getDeviceMinRepeat(int device);
extern int bitMessages_getDeviceRepeat(int device, int repeat);

