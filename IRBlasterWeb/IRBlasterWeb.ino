/*
 IRBlasterWeb
 Accepts http -posts with json named commands and turns them into IR blaster bit bang sequences
 Uses BitMessages to construct messages from named commands and BitTx bit bang library to send them out under interrupt control
 Supports Alexa pin input for muting
 Supports temperature reporting
*/
#define ESP8266
#include "BaseConfig.h"
#define JSON_DOC_SIZE 1500

#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>						
#include "BitMessages.h"
#include "BitTx.h"
#include <ArduinoJson.h>
#if (TEMPERATURE == 1)
	#include <OneWire.h>
	#include <DallasTemperature.h>
#endif

#define INIT_SKIP -1
#define INIT_START 0
#define INIT_DELAY 1
#define INIT_OK1 2
#define INIT_OK2 3
#define INIT_RUN 4
int blasterInit = INIT_START;

int timeInterval = 10;
unsigned long elapsedTime;

#define IR_TIMER 0
int irPin = 14; // 2 or 14
int irFrequency = 38000;

#define MSG_LEN 500 
uint16 msg[MSG_LEN];

//Web command paramters
#define NAME_LEN 16
#define MAX_RECENT 32
char cmdDevice[NAME_LEN];
char cmdParameter[NAME_LEN];
int cmdRepeat, cmdWait = 0, cmdBitCount;
char recentCmds[MAX_RECENT][NAME_LEN];
int recentIndex;
int alexaState = 1; //0 = alexaOn
int alexaPin = 12; // Set to -1 to disable alexa activate processing
int alexaDetect = 0; //Set to 1 to activate Alexa detect handling

WiFiClient client;
HTTPClient cClient;

//Home assistant access
#define MQTT_RETRIES 3
String temperature_topic = "home/sensor/T";
WiFiClient mClient;
PubSubClient mqttClient(mClient);


float diff = 0.1;

void processIr();
void processIrjson();
void saveMacro();
int processJsonCommands(JsonArray& jsData);
int processMacroCommand(String macroName);
int processIrCommand();
void checkStatus();
void addRecentCmd(char* recentCmd);
void recentCommands();
void alexaOn();
void alexaOff();
void sendMsg(int count, int repeat);

//temperature support variales and functions if required
#if (TEMPERATURE == 1)

	#define ONE_WIRE_BUS 13  // DS18B20 pin
	OneWire oneWire(ONE_WIRE_BUS);
	DallasTemperature DS18B20(&oneWire);

	//general variables
	float oldTemp, newTemp;
	unsigned long tempCheckTime;
	unsigned long tempReportTime;
	int tempValid;
	//Timing
	int minMsgInterval = 10; // in units of 1 second
	int forceInterval = 300; // send message after this interval even if temp same 

	/*
	  Check if value changed enough to report
	*/
	bool checkBound(float newValue, float prevValue, float maxDiff) {
		return !isnan(newValue) &&
			 (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
	}

	/*
	  Establish MQTT connection for publishing to Home assistant
	*/
	void mqttConnect() {
		// Loop until we're reconnected
		int retries = 0;
		while (!mqttClient.connected() && (retries < (2 * MQTT_RETRIES))) {
			Serial.print("Attempting MQTT connection...");
			// Attempt to connect
			// If you do not want to use a username and password, change next line to
			// if (mqttClient.connect("ESP8266mqttClient")) {
			if (mqttClient.connect("ESP8266Client", mqtt_user, mqtt_password)) {
				Serial.println("connected");
			} else {
				Serial.print("failed, rc=");
				Serial.println(mqttClient.state());
				delay(300);
				retries++;
				if(retries > MQTT_RETRIES) {
					wifiConnect(1);
				}
			}
		}
	}

	/*
	 Check temperature and report if necessary
	*/
	void checkTemp() {
		if((elapsedTime - tempCheckTime) * timeInterval / 1000 > minMsgInterval) {
			DS18B20.requestTemperatures(); 
			newTemp = DS18B20.getTempCByIndex(0);
			if(newTemp != 85.0 && newTemp != (-127.0)) {
				tempCheckTime = elapsedTime;
				if(checkBound(newTemp, oldTemp, diff) || ((elapsedTime - tempReportTime) * timeInterval / 1000 > forceInterval)) {
					tempReportTime = elapsedTime;
					oldTemp = newTemp;
					Serial.print("New temperature:");
					Serial.println(String(newTemp).c_str());
						
					if (!mqttClient.connected()) {
						mqttConnect();
					}
					if (mqttClient.connected()) {
						mqttClient.loop();
						mqttClient.publish((temperature_topic+macAddr).c_str(), String(newTemp).c_str(), true);
						Serial.println("Published topic:" + temperature_topic+macAddr.c_str());
					} else {
						Serial.println("Not published. MQTT not connected");
					}
				}

			} else {
				Serial.println("Invalid temp reading" + String(newTemp));
				delaymSec(1000);
			}
		}
	}
#endif

/*
Process IR commands with args
*/
void processIr() {
	Serial.println(F("Get args command received"));
	if (server.arg("auth") != AP_AUTHID) {
		Serial.println(F("Unauthorized"));
		server.send(401, "text/html", "Unauthorized");
	} else {
		server.arg("device").toCharArray(cmdDevice, NAME_LEN);
		server.arg("parameter").toCharArray(cmdParameter, NAME_LEN);
		cmdRepeat = server.arg("repeat").toInt();
		cmdWait = server.arg("wait").toInt();
		cmdBitCount = server.arg("bits").toInt();
		server.send(200, "text/html", "Valid args object being processed");
		if(processIrCommand() != 0) {
			char failed[] = "failed";
			addRecentCmd(failed);
		}
	}
}

/*
Process IR commands with json
*/
void processIrjson() {
	DynamicJsonDocument doc(JSON_DOC_SIZE);
	Serial.println(F("Json command received"));
	DeserializationError error = deserializeJson(doc, server.arg("plain"));
	if (error) {
		Serial.println(F("JSON parsing failed"));
		server.send(400, "text/html", "JSON parsing failed");
		return;
	}
	JsonObject jsData = doc.as<JsonObject>();
	if (jsData["auth"].as<String>() != AP_AUTHID) {
		Serial.println(F("Unauthorized"));
		server.send(401, "text/html", "Unauthorized");
	} else {
		server.send(200, "text/html", "Valid JSON command being processed");
		JsonArray jsCommands = jsData["commands"].as<JsonArray>();
		processJsonCommands(jsCommands);
	}
}

void saveMacro() {
	DynamicJsonDocument doc(JSON_DOC_SIZE);
	Serial.println(F("save macro received"));
	DeserializationError error = deserializeJson(doc, server.arg("plain"));
	if (error) {
		Serial.println(F("JSON parsing failed"));
		server.send(400, "text/html", "JSON parsing failed");
		return;
	}
	JsonObject jsData = doc.as<JsonObject>();
	String macro;
	String macroname;
	String json;
	int i;
	if (jsData["auth"].as<String>() != AP_AUTHID) {
		Serial.println(F("Unauthorized"));
		server.send(401, "text/html", "Unauthorized");
	} else {
		macroname = jsData["macro"].as<String>();
		if(macroname.length() > 0) {
			json = jsData["commands"].as<String>();
			if(json.length() > 0) {
				File f = SPIFFS.open("/" + macroname + ".txt", "w");
				f.print(json);
				f.close();
				Serial.print(macroname);
				Serial.println(F(" saved as macro"));
				server.send(200, "text/html", "macro saved OK");
			} else {
				Serial.print(macroname);
				Serial.println(F(" removed if present"));
				SPIFFS.remove("/" + macroname);
				server.send(200, "text/html", "macro removed");
			}
		} else {
			server.send(401, "text/html", "No macro name");
		}
	}
}

/*
Process Json Commands array
*/
int processJsonCommands(JsonArray& jsData) {
	int i;
	Serial.println("command array size " + String(jsData.size()));
	for (i = 0; i < jsData.size(); i++) {
		cmdRepeat = 1;
		strcpy(cmdDevice,jsData[i]["device"].as<const char*>());
		strcpy(cmdParameter,jsData[i]["parameter"].as<const char*>());
		cmdRepeat = jsData[i]["repeat"].as<int>();
		cmdWait = jsData[i]["wait"].as<int>();
		cmdBitCount = jsData[i]["bits"].as<int>();
		if(processIrCommand() != 0) {
			return 2; // invalid command in array
		}
	}
	return 0; //success
}

/*
 Process macro command
*/
int processMacroCommand(String macroName) {
	File f;
	f = SPIFFS.open("/" + macroName + ".txt", "r");
	if(!f) {
		f = SPIFFS.open("/" + macroName, "r");
	}
	if(f) {
		Serial.printf_P(PSTR("Executing macro %s\r\n"), macroName.c_str());
		String json = f.readStringUntil(char(0));
		f.close();
		if(json.length()>2) {
			DynamicJsonDocument doc(JSON_DOC_SIZE);
			DeserializationError error = deserializeJson(doc, json);
			if(error) {
				Serial.println(error.c_str());
			}
			JsonArray jsCommands = doc.as<JsonArray>();
			return processJsonCommands(jsCommands);
		} else {
			Serial.printf_P(PSTR("Macro %s too short\r\n"), macroName.c_str());
			return 1;
		}
	} else {
		Serial.printf_P(PSTR("Macro %s not found\r\n"), macroName.c_str());
		return 1;
	}
}

/*
 Process single IR command
*/
int processIrCommand() {
	int pulseCount = 0, deviceIx;
	int ret = 0;
	
	addRecentCmd(cmdDevice);
	addRecentCmd(cmdParameter);
	if(cmdRepeat == 0) cmdRepeat = 1;
	if(strcasecmp(cmdDevice, "null") == 0) {
		Serial.println(F("Null command"));
		ret = 0;
	} else if(strcasecmp(cmdDevice, "detect") == 0) {
		Serial.printf_P(PSTR("Detect command %s\r\n"),cmdParameter);
		alexaDetect = atoi(cmdParameter);
		ret = 0;
	} else if(strcasecmp(cmdDevice, "macro") == 0) {
		ret = processMacroCommand(String(cmdParameter));
	} else {
		deviceIx = bitMessages_getDevice(cmdDevice);
		if(deviceIx >= 0) {
			cmdRepeat = bitMessages_getDeviceRepeat(deviceIx, cmdRepeat);
			pulseCount = bitMessages_makeNamedMsg(msg, cmdDevice, cmdParameter, cmdRepeat, cmdBitCount);
			if(pulseCount > 0) {
				if(bitMessages_getDeviceFrequency(deviceIx) != irFrequency) {
					irFrequency = bitMessages_getDeviceFrequency(deviceIx);
					Serial.printf_P(PSTR("Changing ir frequency to %d\r\n"), irFrequency);
					bitTx_init(irPin, irFrequency, IR_TIMER);
				}
				Serial.printf_P(PSTR("device %s command %s repeat %d pulses %d\r\n"), cmdDevice, cmdParameter, cmdRepeat, pulseCount);
				sendMsg(pulseCount, cmdRepeat);
				ret = 0;
			} else {
				cmdWait = 0;
				ret = 1;
			}
		} else {
			ret = 2;
			Serial.printf_P(PSTR("Unknown device %s\r\n"), cmdDevice);
		}
	}
	if(cmdWait > 0) {
		Serial.printf_P(PSTR("Waiting %d mSec\r\n"), cmdWait);
		delaymSec(cmdWait);
		cmdWait = 0;
	}
	return ret;
}

/*
 Check status command received from web
*/
void checkStatus() {
	String response;
	Serial.println(F("Check status received"));
	response = "Init:" + String(blasterInit);
	if (alexaPin >= 0) {
		response += "<BR>Alexa:";
		if(alexaDetect == 1)
			response += "On";
		else
			response += "Off";
		if(alexaState == 1)
			response += ":inactive";
		else
			response += ":active";
	}
	
	Dir dir = SPIFFS.openDir("/");
	while (dir.next()) {
		response += "<BR>File:" + dir.fileName() + " - " + dir.fileSize();
	}
	response +="<BR>";
	response += "Free Heap " + String(ESP.getFreeHeap()) +"<BR>";
	
    server.send(200, "text/html", response);
}

/*
 log recent commands
*/
void addRecentCmd(char* recentCmd) {
	strcpy(recentCmds[recentIndex], recentCmd);
	recentIndex++;
	if(recentIndex >= MAX_RECENT) recentIndex = 0;
}

/*
 return recent commands
*/
void recentCommands() {
	String response = "Recent commands<BR>";
	
	Serial.println(F("recent commands request received"));
	int recent = recentIndex;
	for(int i = 0;i<MAX_RECENT;i++) {
		if(strlen(recentCmds[recent]) > 0) {
			response = response + recentCmds[recent] +"<BR>";
		}
		recent++;
		if(recent >= MAX_RECENT) recent = 0;
	}
    server.send(200, "text/html", response);
}

/*
 reload setup
*/
void reload() {
	String response = "Reload devices<BR>";
	Serial.println(F("reload devices request received"));
	response += String(bitMessages_init()) + "<BR>";
    server.send(200, "text/html", response);
}

/*
 Check for Alexa activate on/off
*/
void checkAlexa() {
	int aState = digitalRead(alexaPin);
	if(aState != alexaState && alexaDetect != 0) {
		if(aState == 0)
			alexaOn();
		else
			alexaOff();
	}
	alexaState = aState;
}

/*
 Handle Alexa activate turning on
*/
void alexaOn() {
	processMacroCommand("alexaon");
}

/*
 Handle Alexa activate  turning off
*/
void alexaOff() {
	processMacroCommand("alexaoff");
}

/*
 Send code to IR driver
*/
void sendMsg(int count, int repeat) {
	while(!bitTx_free())
		delaymSec(10);
	Serial.println(F("Transmit IR code"));
	bitTx_send(msg, count, repeat);
}

void setupStart() {
	if(digitalRead(irPin) == 0 && alexaPin >=0 && digitalRead(alexaPin) == 0) {
		blasterInit = INIT_SKIP;
		Serial.println(F("Skip init bitTx"));
	}
}

void extraHandlers() {
	server.on("/irjson" ,processIrjson);
	server.on("/ir", processIr);
	server.on("/macro", saveMacro);
	server.on("/check", checkStatus);
	server.on("/recent", recentCommands);
	server.on("/reload", reload);
}

void setupEnd() {
	Serial.println("Set up IR sender");
	DS18B20.begin();
	mqttClient.setServer(mqtt_server, 1883);																 
	bitTx_init(irPin, irFrequency, IR_TIMER);
	if(alexaPin >=0) {
		pinMode(alexaPin, INPUT_PULLUP);
	}
	if(blasterInit >= INIT_START) {
		Serial.println(bitMessages_init());
		Serial.println(F("Set up complete"));
		blasterInit = INIT_RUN;
	}
}

void loop() {
	server.handleClient();
	if (alexaPin >= 0) checkAlexa();
	#if (TEMPERATURE == 1)
		checkTemp();
	#endif
	delaymSec(timeInterval);
	elapsedTime++;
	wifiConnect(1);
}
