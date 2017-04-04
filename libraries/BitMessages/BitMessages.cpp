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

int bitMessages_addPulses(uint16* msg, char* pulseString) {
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
			msg[count] = round((float)strtoul(p+1, NULL,10) / TICK_PERIOD) + (p[0] == 'H'? 0x8000:0);
			count++;
		}
		p = strtok(NULL, ",");
	}
	//restore token separators
	for(int i=0;i<iLen;i++)
		if(iStr[i] == 0) iStr[i] = ',';
	return count;
}

int bitMessages_addDataPulses(uint16* msg, char* dataString, int bitCount, char* pulses0, char* pulses1) {
	int count=0,i,j,b;
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
		if(c&b)
			count += bitMessages_addPulses(msg+count, pulses1);
		else
			count += bitMessages_addPulses(msg+count, pulses0);
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

//return repeat to use for named device
int bitMessages_getDeviceRepeat(char* deviceString, int repeat) {
	int deviceIndex, locRepeat;
	deviceIndex = bitMessages_getDevice(deviceString);
	if(deviceIndex >=0) {
		if(repeat >= devices[deviceIndex].minRepeat)
			return repeat;
		else
			return devices[deviceIndex].minRepeat;
	} else {
		return repeat;
	}
}

// return device data functions
int bitMessages_getDeviceCount() {return NUMBER_DEVICES;}
char* bitMessages_getDeviceDataDeviceName(int device) {return devices[device].deviceName;}
char* bitMessages_getDeviceDataHeader(int device) {return devices[device].header;}
char* bitMessages_getDeviceDataTrailer(int device) {return devices[device].trailer;}
char* bitMessages_getDeviceDataPulses0(int device) {return devices[device].pulses0;}
char* bitMessages_getDeviceDataPulses1(int device) {return devices[device].pulses1;}
char* bitMessages_getDeviceDataButton(int device, int button) {return devices[device].buttons[button];}
int bitMessages_getDeviceBitCount(int device) {return devices[device].bitCount;}
int bitMessages_getDeviceRepeatDelay(int device) {return devices[device].repeatDelay;}
int bitMessages_getDeviceMinRepeat(int device) {return devices[device].minRepeat;}


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
int bitMessages_makeMsg(uint16* msg, char* header, char* trailer, char* dataString, int bitCount, char* pulses0, char* pulses1, int delay) {
	int count = 0;
	if(header != NULL) 
		count += bitMessages_addPulses(msg, header);
	if(dataString != NULL) 
		count += bitMessages_addDataPulses((msg+count), dataString, bitCount, pulses0, pulses1);
	if(trailer != NULL) 
		count += bitMessages_addPulses((msg+count), trailer);
	if(delay > 0)
		count += bitMessages_addDelay((msg+count), delay);
	return count;
}

// Make message from device and button numbers
int bitMessages_makeIndexedMsg(uint16* msg, int device, int button, int repeat) {
	int delay = 0;
	if(repeat > 1 || devices[device].minRepeat > 1)
		delay = devices[device].repeatDelay;
	return bitMessages_makeMsg(msg, devices[device].header, devices[device].trailer, devices[device].buttons[button],
								devices[device].bitCount, devices[device].pulses0, devices[device].pulses1, delay);
}

// Make message from device name and button name
int bitMessages_makeNamedMsg(uint16* msg, char* deviceString, char* buttonString, int repeat) {
	int deviceIndex, buttonIndex, locRepeat;
	deviceIndex = bitMessages_getDevice(deviceString);
	if(deviceIndex >=0) {
		buttonIndex = bitMessages_getButton(deviceIndex, buttonString);
		if(buttonIndex >=0) {
			return bitMessages_makeIndexedMsg(msg, deviceIndex, buttonIndex, repeat);
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}