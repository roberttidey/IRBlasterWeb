// BitMessages.h
//
// Defines Messages and pulse periods to use with BitTx bit bang library
// raw msg format string is csv with elements h:period,l:period,1:bbb(binary),4:hhh(hex)
// data bits from 1: or 4: are represented from a period array 00001111 where up to 4 pulses are allowed for 0 and 4 for a 1
// if a period is 0 then it is ignored.
// Messages can be obtained from device name and button definitions. These are initialised from files in SPIFFS

//Attempt to decode pulse timings from BitRX pulse capture library into common IR protocols (nec,rc5,rc6)
//Updates a device structure
//Can save updated structure back to SPIFFS

// Author: Bob Tidey (robert@tideys.net)
#include <c_types.h>

#define TICK_PERIOD 3.2
#define HEX_ESCAPE '%'
#define COUNT_ESCAPE '#'

//bit definitions for special handling when adding data pulses
#define SPECIAL_NORMAL 0x0000
#define SPECIAL_RC6 0x0001
#define SPECIAL_TOGGLE 0x0002

#define MAX_DEVICES 6
#define MAX_FIELD_LENGTH 24
#define MAX_BUTTONNAMES 160
#define MAX_BUTTONNAME_LENGTH 16
#define MAX_CODE_LENGTH 3000
#define NULL_BUTTON 0xFFFF
#define BUTTON_FILE "/buttonnames.txt"

struct deviceData {
	char deviceFile[MAX_FIELD_LENGTH];
	char deviceName[MAX_FIELD_LENGTH];
	char header[MAX_FIELD_LENGTH];
	char trailer[MAX_FIELD_LENGTH];
	char pulses0[MAX_FIELD_LENGTH];
	char pulses1[MAX_FIELD_LENGTH];
	int frequency;
	int special;
	int repeatDelay; //mSec
	int bitCount;
	int minRepeat; //repeat 1 is send once
	int toggle; // holds toggle state if used
	char necAddress[8]; // holds nec Address 
	int spare1; // spare
	short buttonIxs[MAX_BUTTONNAMES];
};

// Initialisation and load
extern char* bitMessages_init();
extern int bitMessages_loadDevice(int deviceIndex, char* filename);

// make message pulse buffer routines
extern int bitMessages_addPulses(uint16* msg, char* pulseString, int factor);
extern int bitMessages_addDataPulses(uint16* msg, char* dataString, int bitCount, char* pulses0, char* pulses1, int special);
extern int bitMessages_getDevice(char* deviceString);
extern int bitMessages_getButton(char* buttonString);
extern int bitMessages_addDelay(uint16* msg, int delay);
extern int bitMessages_makeMsg(uint16* msg, char* header, char* trailer, char* dataString, int bitCount, char* pulses0, char* pulses1, int special, int repeatDelay);
extern int bitMessages_makeNamedMsg(uint16* msg, char* deviceString, char* buttonString, int repeat, int bits);

//DeviceData access routines
extern int bitMessages_getDeviceCount();
extern char* bitMessages_getDeviceDataDeviceFile(int device);
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

//RX message update routines
extern int bitMessages_saveDeviceData(int device);
extern int bitMessages_readTryNEC(uint16* buf);
extern int bitMessages_readTryRC5(uint16* buf);
extern int bitMessages_readTryRC6(uint16* buf);
extern int bitMessages_readDeviceButton(int device, int button, uint16* buf, int updateHdr);


