/*
 IRBlasterWeb
 Accepts http -posts with json named commands and turns them into IR blaster bit bang sequences
 Uses BitMessages to construct messages from named commands and BitTx bit bang library to send them out under interrupt control
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "BitMessages.h"
#include "BitTx.h"
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include "FS.h"

/*
 Web set up
*/

#define AP_SSID "ssid"
#define AP_PASSWORD "password"
#define AP_MAX_WAIT 10
#define AP_PORT 80

//uncomment next line to use static ip address instead of dhcp
//#define AP_IP 192,168,0,200
#define AP_DNS 192,168,0,1
#define AP_GATEWAY 192,168,0,1
#define AP_SUBNET 255,255,255,0

#define AP_AUTHID "1234"

#define IR_TIMER 0
int irPin = 14; // 2 or 14
int irFrequency = 38000;

//For update service
String host = "esp8266-irblaster";
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "password";

#define MSG_LEN 500 
uint16 msg[MSG_LEN];

//Web command paramters
#define NAME_LEN 16 
char cmdDevice[NAME_LEN];
char cmdParameter[NAME_LEN];
int cmdRepeat, cmdWait = 0, cmdBitCount;

ESP8266WebServer server(AP_PORT);
ESP8266HTTPUpdateServer httpUpdater;

char* mainPage1 = "Main page goes here";
const char mainPage[] =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
"<title>ESP8266 Web Form Demo</title>"
"<style>"
"\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
"</style>"
"</head>"
"<body>"
"<h1>IR Blaster</h1>"
"<FORM action=\"/ir\" method=\"post\">"
"<P>"
"Command<br>"
"<INPUT type=\"text\" name=\"auth\" value=\"\">PinCode<BR>"
"<INPUT type=\"text\" name=\"device\" value=\"yamahaAV\">Device<BR>"
"<INPUT type=\"text\" name=\"parameter\" value=\"play\">Button<BR>"
"<INPUT type=\"text\" name=\"bits\" value=\"0\">BitCount (0 default)<BR>"
"<INPUT type=\"text\" name=\"repeat\" value=\"1\">Repeat<BR>"
"<INPUT type=\"text\" name=\"wait\" value=\"100\">Wait mSec<BR>"
"<INPUT type=\"submit\" value=\"Send\">"
"</P>"
"</FORM>"
"</body>"
"</html>";

/*
 Initialise wifi, message handlers and ir sender
*/
void setup() {
	Serial.begin(115200);
	Serial.println("Set up IRBlaster Web");
	wifiConnect();
	
	//Update service
	Serial.println("Set up Web update service");
	MDNS.begin(host.c_str());
	httpUpdater.setup(&server, update_path, update_username, update_password);
	MDNS.addService("http", "tcp", AP_PORT);

	Serial.println("Set up Web command handlers");
	server.on("/irjson" ,processIrjson);
	server.on("/ir", processIr);
	server.on("/macro", saveMacro);
	server.on("/check", checkStatus);
	server.on("/", indexHTML);
	server.begin();
	Serial.println("Set up IR sender");
	bitTx_init(irPin, irFrequency, IR_TIMER);
	Serial.println("Set up filing system");
	initFS();
	
	Serial.println("Set up complete");
}

/*
  Connect to local wifi with retries
*/
int wifiConnect()
{
	int retries = 0;
	Serial.print("Connecting to AP");
#ifdef AP_IP
	IPAddress addr1(AP_IP);
	IPAddress addr2(AP_DNS);
	IPAddress addr3(AP_GATEWAY);
	IPAddress addr4(AP_SUBNET);
	WiFi.config(addr1, addr2, addr3, addr4);
#endif
	WiFi.begin(AP_SSID, AP_PASSWORD);
	while (WiFi.status() != WL_CONNECTED && retries < AP_MAX_WAIT) {
		delay(1000);
		Serial.print(".");
		retries++;
	}
	Serial.println("");
	if(retries < AP_MAX_WAIT) {
		Serial.print("WiFi connected ip ");
		Serial.print(WiFi.localIP());
		Serial.printf(":%d mac %s\r\n", AP_PORT, WiFi.macAddress().c_str());
		return 1;
	} else {
		Serial.println("WiFi connection attempt failed"); 
		return 0;
	} 
}

void initFS() {
	SPIFFS.begin();
	File f = SPIFFS.open("/wait5", "r");
	if(!f) {
		Serial.println("No wait5 file present. Init SIFFS using Arduino tool");
	} else {
		f.close();
		Serial.println("SPIFFS file list");
		Dir dir = SPIFFS.openDir("/");
		while (dir.next()) {
			Serial.print(dir.fileName());
			Serial.print(" - ");
			Serial.println(dir.fileSize());
		}
	}
}

/*
Process IR commands with args
*/
void processIr() {
	Serial.println("Get args command received");
	if (server.arg("auth") != AP_AUTHID) {
		Serial.println("Unauthorized");
		server.send(401, "text/html", "Unauthorized");
	} else {
		server.arg("device").toCharArray(cmdDevice, NAME_LEN);
		server.arg("parameter").toCharArray(cmdParameter, NAME_LEN);
		cmdRepeat = server.arg("repeat").toInt();
		cmdWait = server.arg("wait").toInt();
		cmdBitCount = server.arg("bits").toInt();
		if(processIrCommand() == 0)
			server.send(200, "text/html", "Valid args object being processed");
		else
			server.send(400, "text/html", "Failed");
	}
}

/*
Process IR commands with json
*/
void processIrjson() {
	DynamicJsonBuffer jsonBuf;
	Serial.println("Json command received");
/*
	for(int i=0;i<server.args();i++) {
		Serial.println(server.argName(i));
		Serial.println(server.arg(i));
	}
*/
	JsonObject& jsData = jsonBuf.parseObject(server.arg("plain"));
	
	if (!jsData.success()) {
		Serial.println("JSON parsing failed");
		server.send(400, "text/html", "JSON parsing failed");
	} else if (jsData.get<String>("auth") != AP_AUTHID) {
		Serial.println("Unauthorized");
		server.send(401, "text/html", "Unauthorized");
	} else {
		server.send(200, "text/html", "Valid JSON command being processed");
		JsonArray& jsCommands = jsData.get<JsonVariant>("commands").as<JsonArray&>();
		processJsonCommands(jsCommands);
	}
}

void saveMacro() {
	DynamicJsonBuffer jsonBuf;
	JsonObject& jsData = jsonBuf.parseObject(server.arg("plain"));
	String macro;
	String macroname;
	String json;
	int i;
	
	Serial.println("save macro received");
	if (!jsData.success()) {
		Serial.println("JSON parsing failed");
		server.send(400, "text/html", "JSON parsing failed");
	} else if (jsData.get<String>("auth") != AP_AUTHID) {
		Serial.println("Unauthorized");
		server.send(401, "text/html", "Unauthorized");
	} else {
		macroname = jsData.get<String>("macro");
		if(macroname.length() > 0) {
			json = jsData.get<String>("commands");
			if(json.length() > 0) {
				File f = SPIFFS.open("/" + macroname, "w");
				f.print(json);
				f.close();
				Serial.print(macroname);
				Serial.println(" saved as macro");
				server.send(200, "text/html", "macro saved OK");
			} else {
				Serial.print(macroname);
				Serial.println(" removed if present");
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
	
	for (i = 0; i < jsData.size(); i++) {
		cmdRepeat = 1;
		strcpy(cmdDevice,jsData[i]["device"].asString());
		strcpy(cmdParameter,jsData[i]["parameter"].asString());
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
 Process single IR command
*/
int processIrCommand() {
	int pulseCount = 0, deviceIx;
	int ret = 0;
	
	if(cmdRepeat == 0) cmdRepeat = 1;
	if(strcmpi(cmdDevice, "null") == 0) {
		Serial.println("Null command");
		ret = 0;
	} else if(strcmpi(cmdDevice, "macro") == 0) {
		File f = SPIFFS.open("/" + String(cmdParameter), "r");
		if(f) {
			Serial.printf("Executing macro %s\r\n", cmdParameter);
			String json = f.readStringUntil(char(0));
			f.close();
			if(json.length()>2) {
				DynamicJsonBuffer jsonBuf;
				JsonArray& jsCommands = jsonBuf.parseArray(json);
				ret = processJsonCommands(jsCommands);
			} else {
				Serial.printf("Macro %s too short\r\n", cmdParameter);
				ret = 1;
			}
		} else {
			Serial.printf("Macro %s not found\r\n", cmdParameter);
			ret = 1;
		}
	} else {
		deviceIx = bitMessages_getDevice(cmdDevice);
		if(deviceIx >= 0) {
			cmdRepeat = bitMessages_getDeviceRepeat(deviceIx, cmdRepeat);
			pulseCount = bitMessages_makeNamedMsg(msg, cmdDevice, cmdParameter, cmdRepeat, cmdBitCount);
			if(pulseCount > 0) {
				if(bitMessages_getDeviceFrequency(deviceIx) != irFrequency) {
					irFrequency = bitMessages_getDeviceFrequency(deviceIx);
					Serial.printf("Changing ir frequency to %d\r\n", irFrequency);
					bitTx_init(irPin, irFrequency, IR_TIMER);
				}
				Serial.printf("device %s command %s repeat %d pulses %d\r\n", cmdDevice, cmdParameter, cmdRepeat, pulseCount);
				sendMsg(pulseCount, cmdRepeat);
				ret = 0;
			} else {
				cmdWait = 0;
				ret = 1;
			}
		} else {
			ret = 2;
			Serial.printf("Unknown device %s\r\n", cmdDevice);
		}
	}
	if(cmdWait > 0) {
		Serial.printf("Waiting %d mSec\r\n", cmdWait);
		delay(cmdWait);
		cmdWait = 0;
	}
	return ret;
}

/*
 Check status command received from web
*/
void indexHTML() {
Serial.println("Main page requested");
    server.send(200, "text/html", mainPage);
}

/*
 Check status command received from web
*/
void checkStatus() {
	String response = "IR Blaster is running<BR>Macros<BR>";
	
	Serial.println("Check status received");
	Dir dir = SPIFFS.openDir("/");
	while (dir.next()) {
		response += dir.fileName() + " - " + dir.fileSize() + "<BR>";
	}
	
    server.send(200, "text/html", response);
}

/*
 Send code to IR driver
*/
void sendMsg(int count, int repeat) {
	while(!bitTx_free())
		delay(10);
	Serial.println("Transmit IR code");
	bitTx_send(msg, count, repeat);
}

void loop() {
	server.handleClient();
	delay(10);
}
