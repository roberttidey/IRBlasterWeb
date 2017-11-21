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
#include "FS.h"

deviceData devices[MAX_DEVICES];
char buttonNames[MAX_BUTTONNAMES][MAX_BUTTONNAME_LENGTH];
char codes[MAX_CODE_LENGTH];
int number_devices;
int number_buttonNames;
char* pCode;
char retMsg[64];

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

int bitMessages_addDataPulses(uint16* msg, char* dataString, int bitCount, char* pulses0, char* pulses1, int deviceIx) {
	int count=0,i,j,b, factor=1;
	char c;
	char buffer[MAX_FIELD_LENGTH];
	
	if(dataString == NULL) {
		return 0;
	}
	//add a fixed  NEC code if defined and data is just 4 hex characters
	if(strlen(devices[deviceIx].necAddress) == 4 && strlen(dataString) == 4) {
		strcpy(buffer, devices[deviceIx].necAddress);
		strcpy(buffer+4, dataString);
	} else {
		strcpy(buffer, dataString);
	}
	i=0;
	b = 0;
	for(j=0;j<bitCount;j++) {
		if(b==0) {
			if(i<strlen(buffer)) {
				c = buffer[i];
				i++;
			} else {
				c = '0';
			}
			c = (c > '9')? (c &~ 0x20) - 'A' + 10: (c - '0');
			b = 0x08;
		}
		// handle any special requirements
		if(devices[deviceIx].special & SPECIAL_RC6 !=0) factor = (j!=3) ? 1 : 2;
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
	for(i = 0; i < number_devices; i++) {
		if(strcasecmp(devices[i].deviceName, deviceString) == 0)
			return i;
	}
	return -1;
}

// Find index of matching button name
int bitMessages_getButton(char* buttonString) {
	int i;
	for(i=0;i<number_buttonNames;i++) {
		if(strcasecmp(buttonNames[i], buttonString) == 0)
			return i;
	}
	return -1;
}


// return device data functions
int bitMessages_getDeviceCount() {return number_devices;}
char* bitMessages_getDeviceDataDeviceName(int device) {return devices[device].deviceName;}
char* bitMessages_getDeviceDataHeader(int device) {return devices[device].header;}
char* bitMessages_getDeviceDataTrailer(int device) {return devices[device].trailer;}
char* bitMessages_getDeviceDataPulses0(int device) {return devices[device].pulses0;}
char* bitMessages_getDeviceDataPulses1(int device) {return devices[device].pulses1;}
int bitMessages_getDeviceFrequency(int device) {return devices[device].frequency;}
int bitMessages_getDeviceSpecial(int device) {return devices[device].special;}
int bitMessages_getDeviceBitCount(int device) {return devices[device].bitCount;}
int bitMessages_getDeviceRepeatDelay(int device) {return devices[device].repeatDelay;}
int bitMessages_getDeviceMinRepeat(int device) {return devices[device].minRepeat;}
char* bitMessages_getDeviceDataButton(int device, int button) {
	short buttonIx = devices[device].buttonIxs[button];
	if(buttonIx == NULL_BUTTON) return 0;
	return codes + buttonIx;
}

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
int bitMessages_makeMsg(uint16* msg, char* header, char* trailer, char* dataString, int bitCount, char* pulses0, char* pulses1, int deviceIx, int delay) {
	int count = 0;
	if(header != NULL) 
		count += bitMessages_addPulses(msg, header, 1);
	if(dataString != NULL) 
		count += bitMessages_addDataPulses((msg+count), dataString, bitCount, pulses0, pulses1, deviceIx);
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
		// set default bit count
		bCount = devices[deviceIx].bitCount;
		// Check for passing data through as hex string
		if(buttonString[0] == HEX_ESCAPE) {
			button = buttonString+1;
		} else {
			buttonIx = bitMessages_getButton(buttonString);
			if (buttonIx < 0) return 0;
			if(buttonIx >= MAX_BUTTONNAMES) return 0;
			button = bitMessages_getDeviceDataButton(deviceIx, buttonIx);
			if(button == NULL) return 0;
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
								bCount, devices[deviceIx].pulses0, devices[deviceIx].pulses1, deviceIx, delay);
	} else {
		return 0;
	}
}

//load remote control data from a file, returns number of codes loaded.
int bitMessages_loadDevice(int deviceIx, char* filename){
	int i;
	int button;
	int buttonIx = -1;;
	String line;
	String temp;
	char lineC[MAX_FIELD_LENGTH];
	int sep;
	
	for(i=0;i<number_buttonNames;i++)
		devices[deviceIx].buttonIxs[i] = NULL_BUTTON;
	File f = SPIFFS.open(String(filename), "r");
	i=0;
	while(f.available()) {
		line =f.readStringUntil('\n');
		line.replace("\r","");
		if(line == "NULL") line = "";
		if(line.charAt(0) != '#') {
			if(buttonIx >=0) {
				sep = line.indexOf(',');
				if(sep >=0) {
					temp = line.substring(0,sep);
					temp.trim();
					strcpy(lineC, temp.c_str());
					button = bitMessages_getButton(lineC);
					if(button >= 0) {
						temp = line.substring(sep+1);
						temp.trim();
						strcpy(pCode, temp.c_str());
						devices[deviceIx].buttonIxs[button] = pCode - codes;
						buttonIx++;
						pCode += temp.length() + 1;
					}
				}
			} else {
				switch (i) {
					case 0: strcpy(devices[deviceIx].deviceName, line.c_str());break;
					case 1: strcpy(devices[deviceIx].header, line.c_str());break;
					case 2: strcpy(devices[deviceIx].trailer, line.c_str());break;
					case 3: strcpy(devices[deviceIx].pulses0, line.c_str());break;
					case 4: strcpy(devices[deviceIx].pulses1, line.c_str());break;
					case 5: devices[deviceIx].frequency = line.toInt();break;
					case 6: devices[deviceIx].special = line.toInt();break;
					case 7: devices[deviceIx].repeatDelay = line.toInt();break;
					case 8: devices[deviceIx].bitCount = line.toInt();break;
					case 9: devices[deviceIx].minRepeat = line.toInt();break;
					case 10: devices[deviceIx].toggle = line.toInt();break;
					case 11: strcpy(devices[deviceIx].necAddress,line.c_str());break;
					case 12: devices[deviceIx].spare1 = line.toInt();buttonIx=0;break;
				}
				i++;
			}
		}
	}
	f.close();
	return buttonIx;
}

//Initialisation - load all remote control data from filing system
char* bitMessages_init(){
	String ret;
	String line;
	int temp_codes;
	int total_codes = 0;
	char temp[MAX_FIELD_LENGTH];
	pCode = codes;
	number_devices = 0;
	number_buttonNames = 0;
	//load buttonnames
	File f = SPIFFS.open(BUTTON_FILE, "r");
	while(f.available()) {
		line =f.readStringUntil('\n');
		if(line.length() > 0 && line.charAt(0) != '#') {
			strcpy(buttonNames[number_buttonNames], line.c_str());
			number_buttonNames++;
		}
	}
	f.close();
	//iterate over files starting dev_
	Dir dir = SPIFFS.openDir("/");
	while (dir.next()  && (number_devices < (MAX_DEVICES - 1))) {
		line = String(dir.fileName());
		if(line.indexOf("/dev_") == 0) {
			strcpy(temp, line.c_str());
			temp_codes = bitMessages_loadDevice(number_devices, temp);
			total_codes += temp_codes;
			if(temp_codes)
				number_devices++;
		}
	}
	sprintf(retMsg, "Buttons:%d Devices:%d Codes: %d Length:%d",number_buttonNames,number_devices,total_codes,pCode - codes);
	return retMsg;
}