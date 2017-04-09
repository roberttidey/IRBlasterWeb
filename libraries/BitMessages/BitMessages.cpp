// BitMessages.cpp
//
// Generates pulses into a buffer to use with BitTx bit bang library
// Total message is made from header, dtat and trailer using pulse definition strings
// pulse definition string is csv with elements h:period,l:period where period is in uSec
// data bits are from a hex string using pulse0 and pulse1 strings to define how to represent 0/1
// Author: Bob Tidey (robert@tideys.net)

#include "BitMessages.h"
#include <arduino.h>
#include <string.h>

#include "BitDevices.h"

int bitMessages_addPulses(uint16* msg, char* pulseString, int factor) {
	int count=0;
	char* iStr = pulseString;
	int iLen = strlen(pulseString);
	char* p;
	if(pulseString == NULL) {
		return 0;
	}
	p = strtok(pulseString, ",");
	while(p != NULL) {
		if(p[0] == 'H' || p[0] == 'L') {
			msg[count] = round((float)strtoul(p+1, NULL,10) / TICK_PERIOD) * factor + (p[0] == 'H'? 0x8000:0);
			count++;
		}
		p = strtok(NULL, ",");
	}
	//restore token separators
	for(int i=0;i<iLen;i++)
		if(iStr[i] == 0) iStr[i] = ',';
	return count;
}

int bitMessages_addDataPulses(uint16* msg, char* dataString, int bitCount, char* pulses0, char* pulses1, int special) {
	int count=0,i,j,b, factor=1;
	char c;
	if(dataString == NULL) {
		return 0;
	}
	i=0;
	b = 0;
	for(j=0;j<bitCount;j++) {
		if(b==0) {
			if(i<strlen(dataString)) {
				c = dataString[i];
				i++;
			} else {
				c = '0';
			}
			c = (c > '9')? (c &~ 0x20) - 'A' + 10: (c - '0');
			b = 0x08;
		}
		// handle any special requirements
		if(special & SPECIAL_RC6 !=0) factor = (j!=3) ? 1 : 2;
		if(c&b)
			count += bitMessages_addPulses(msg+count, pulses1, factor);
		else
			count += bitMessages_addPulses(msg+count, pulses0, factor);
		b>>=1;
	}
	return count;
}

// Find structure for a device
int bitMessages_getDevice(char* deviceString) {
	int i;
	for(i = 0; i < NUMBER_DEVICES; i++) {
		if(strcmpi(devices[i].deviceName, deviceString) == 0)
			return i;
	}
	return -1;
}

// Find matching button in device
int bitMessages_getButton(int device, char* buttonString) {
	int i=0;
	while(devices[device].buttons[i] != NULL) {
		if(strcmpi(devices[device].buttons[i], buttonString) == 0)
			return i+1;
		i+=2;
	}
	return -1;
}


// return device data functions
int bitMessages_getDeviceCount() {return NUMBER_DEVICES;}
char* bitMessages_getDeviceDataDeviceName(int device) {return devices[device].deviceName;}
char* bitMessages_getDeviceDataHeader(int device) {return devices[device].header;}
char* bitMessages_getDeviceDataTrailer(int device) {return devices[device].trailer;}
char* bitMessages_getDeviceDataPulses0(int device) {return devices[device].pulses0;}
char* bitMessages_getDeviceDataPulses1(int device) {return devices[device].pulses1;}
char* bitMessages_getDeviceDataButton(int device, int button) {return devices[device].buttons[button];}
int bitMessages_getDeviceFrequency(int device) {return devices[device].frequency;}
int bitMessages_getDeviceSpecial(int device) {return devices[device].special;}
int bitMessages_getDeviceBitCount(int device) {return devices[device].bitCount;}
int bitMessages_getDeviceRepeatDelay(int device) {return devices[device].repeatDelay;}
int bitMessages_getDeviceMinRepeat(int device) {return devices[device].minRepeat;}

//return repeat to use  device
int bitMessages_getDeviceRepeat(int device, int repeat) {
	if(device >=0) {
		if(repeat >= devices[device].minRepeat)
			return repeat;
		else
			return devices[device].minRepeat;
	} else {
		return repeat;
	}
}

//Add delay to message buffer
int bitMessages_addDelay(uint16* msg, int delay) {
	int count;
	long delayTicks = delay * 1000 / TICK_PERIOD;
	while(delayTicks > 32767) {
		msg[count++] = 32767;
		delayTicks -= 32767;
	}
	if(delayTicks > 0)
		msg[count++] = delayTicks;
	
	return count;
}

//Make message from header, data and trailer strings
int bitMessages_makeMsg(uint16* msg, char* header, char* trailer, char* dataString, int bitCount, char* pulses0, char* pulses1, int special, int delay) {
	int count = 0;
	if(header != NULL) 
		count += bitMessages_addPulses(msg, header, 1);
	if(dataString != NULL) 
		count += bitMessages_addDataPulses((msg+count), dataString, bitCount, pulses0, pulses1, special);
	if(trailer != NULL) 
		count += bitMessages_addPulses((msg+count), trailer, 1);
	if(delay > 0)
		count += bitMessages_addDelay((msg+count), delay);
	return count;
}


// Make message from device name and button name
int bitMessages_makeNamedMsg(uint16* msg, char* deviceString, char* buttonString, int repeat, int bits) {
	int deviceIx, buttonIx, delay = 0, bCount;
	char* button;
	char* bitPrefix;
	
	deviceIx = bitMessages_getDevice(deviceString);
	if(deviceIx >=0) {
		// set default bit vount
		bCount = devices[deviceIx].bitCount;
		// Check for passing data through as hex string
		if(buttonString[0] == HEX_ESCAPE) {
			button = buttonString+1;
		} else {
			buttonIx = bitMessages_getButton(deviceIx, buttonString);
			if (buttonIx <= 0) return 0;
			button = devices[deviceIx].buttons[buttonIx];
		}
		
		//Check for string starting with COUNT_ESCAPE
		if(button[0] == COUNT_ESCAPE) {
			// Search for matching end bit prefix and convert to a bit count
			bitPrefix = strchr(button+1, COUNT_ESCAPE);
			if(bitPrefix != NULL) {
				bitPrefix[0] = 0;
				bCount = strtoul(button+1, NULL, 10);
				bitPrefix[0] = COUNT_ESCAPE;
				button = bitPrefix + 1;
			} else {
				// no matching end escape so ignore it
				button++;
			}
		}
		
		// Override if supplied bits non zero
		if(bits != 0) bCount = bits;
		
		if(repeat > 1 || devices[deviceIx].minRepeat > 1)
			delay = devices[deviceIx].repeatDelay;
		return bitMessages_makeMsg(msg, devices[deviceIx].header, devices[deviceIx].trailer, button,
								bCount, devices[deviceIx].pulses0, devices[deviceIx].pulses1, devices[deviceIx].special, delay);
	} else {
		return 0;
	}
}