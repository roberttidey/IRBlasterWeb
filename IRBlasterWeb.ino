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
#include <DNSServer.h>
#include <WiFiManager.h>

/*
 Manual Web set up
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

/*
Wifi Manager Web set up
If WM_NAME defined then use WebManager
*/
#define WM_NAME "irWebSetup"
#define WM_PASSWORD "password"
#ifdef WM_NAME
	WiFiManager wifiManager;
#endif

int timeInterval = 10;
#define WIFI_CHECK_TIMEOUT 30000
unsigned long elapsedTime;
unsigned long wifiCheckTime;

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
#define MAX_RECENT 32
char cmdDevice[NAME_LEN];
char cmdParameter[NAME_LEN];
int cmdRepeat, cmdWait = 0, cmdBitCount;
char recentCmds[MAX_RECENT][NAME_LEN];
int recentIndex;
int alexaState = 1; //0 = alexaOn
int alexaPin = 13; // Set to -1 to disable alexa activate processing
int alexaDetect = 0; //Set to 1 to activate Alexa detect handling

ESP8266WebServer server(AP_PORT);
ESP8266HTTPUpdateServer httpUpdater;

//holds the current upload
File fsUploadFile;

int wifiConnect(int check);
void initFS();
String getContentType(String filename);
bool handleFileRead(String path);
void handleFileUpload();
void handleFileDelete();
void handleFileCreate();
void handleFileList();
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

void ICACHE_RAM_ATTR  delaymSec(unsigned long mSec) {
	unsigned long ms = mSec;
	while(ms > 100) {
		delay(100);
		ms -= 100;
		ESP.wdtFeed();
	}
	delay(ms);
	ESP.wdtFeed();
	yield();
}

void ICACHE_RAM_ATTR  delayuSec(unsigned long uSec) {
	unsigned long us = uSec;
	while(us > 100000) {
		delay(100);
		us -= 100000;
		ESP.wdtFeed();
	}
	delayMicroseconds(us);
	ESP.wdtFeed();
	yield();
}

/*
  Connect to local wifi with retries
  If check is set then test the connection and re-establish if timed out
*/
int wifiConnect(int check) {
	if(check) {
		if((elapsedTime - wifiCheckTime) * timeInterval > WIFI_CHECK_TIMEOUT) {
			if(WiFi.status() != WL_CONNECTED) {
				Serial.println(F("Wifi connection timed out. Try to relink"));
			} else {
				wifiCheckTime = elapsedTime;
				return 0;
			}
		} else {
			return 0;
		}
	}
	wifiCheckTime = elapsedTime;
#ifdef WM_NAME
	Serial.println(F("Set up managed IRBlaster Web"));
	if(check == 0) {
		wifiManager.setConfigPortalTimeout(180);
		wifiManager.autoConnect(WM_NAME, WM_PASSWORD);
	} else {
		WiFi.begin();
	}
#else
	Serial.println(F("Set up manual IRBlaster Web"));
	int retries = 0;
	Serial.print(F("Connecting to AP"));
	#ifdef AP_IP
		IPAddress addr1(AP_IP);
		IPAddress addr2(AP_DNS);
		IPAddress addr3(AP_GATEWAY);
		IPAddress addr4(AP_SUBNET);
		WiFi.config(addr1, addr2, addr3, addr4);
	#endif
	WiFi.begin(AP_SSID, AP_PASSWORD);
	while (WiFi.status() != WL_CONNECTED && retries < AP_MAX_WAIT) {
		delaymSec(1000);
		Serial.print(F(("."));
		retries++;
	}
	Serial.println("");
	if(retries < AP_MAX_WAIT) {
		Serial.print(F("WiFi connected ip "));
		Serial.print(WiFi.localIP());
		Serial.printf_P(PSTR(":%d mac %s\r\n"), AP_PORT, WiFi.macAddress().c_str());
		return 1;
	} else {
		Serial.println(F("WiFi connection attempt failed"));
		return 0;
	} 
#endif
}

void initFS() {
	SPIFFS.begin();
	if(!SPIFFS.begin()) {
		Serial.println(F("No SIFFS found. Format it"));
		if(SPIFFS.format()) {
			SPIFFS.begin();
		} else {
			Serial.println(F("No SIFFS found. Format it"));
		}
	} else {
		Serial.println(F("SPIFFS file list"));
		Dir dir = SPIFFS.openDir("/");
		while (dir.next()) {
			Serial.print(dir.fileName());
			Serial.print(F(" - "));
			Serial.println(dir.fileSize());
		}
	}
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  Serial.printf_P(PSTR("handleFileRead: %s\r\n"), path.c_str());
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.printf_P(PSTR("handleFileUpload Name: %s\r\n"), filename.c_str());
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    Serial.printf_P(PSTR("handleFileUpload Data: %d\r\n"), upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    Serial.printf_P(PSTR("handleFileUpload Size: %d\r\n"), upload.totalSize);
  }
}

void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.printf_P(PSTR("handleFileDelete: %s\r\n"),path.c_str());
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.printf_P(PSTR("handleFileCreate: %s\r\n"),path.c_str());
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = server.arg("dir");
  Serial.printf_P(PSTR("handleFileList: %s\r\n"),path.c_str());
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  output += "]";
  server.send(200, "text/json", output);
}

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
	Serial.println(F("Json command received"));
/*
	for(int i=0;i<server.args();i++) {
		Serial.println(server.argName(i));
		Serial.println(server.arg(i));
	}
*/
	JsonObject& jsData = jsonBuf.parseObject(server.arg("plain"));
	
	if (!jsData.success()) {
		Serial.println(F("JSON parsing failed"));
		server.send(400, "text/html", "JSON parsing failed");
	} else if (jsData.get<String>("auth") != AP_AUTHID) {
		Serial.println(F("Unauthorized"));
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
	
	Serial.println(F("save macro received"));
	if (!jsData.success()) {
		Serial.println(F("JSON parsing failed"));
		server.send(400, "text/html", "JSON parsing failed");
	} else if (jsData.get<String>("auth") != AP_AUTHID) {
		Serial.println(F("Unauthorized"));
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
			DynamicJsonBuffer jsonBuf;
			JsonArray& jsCommands = jsonBuf.parseArray(json);
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
	String response = "IR Blaster is running<BR>";
	Serial.println(F("Check status received"));
	if (alexaPin >= 0) {
		response += "Alexa: Detect ";
		if(alexaDetect == 1)
			response += "On ";
		else
			response += "Off ";
		if(alexaState == 1)
			response += "inactive";
		else
			response += "active";
	}
	response += "<BR>Macros<BR>";
	
	Dir dir = SPIFFS.openDir("/");
	while (dir.next()) {
		response += dir.fileName() + " - " + dir.fileSize() + "<BR>";
	}
	
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

/*
 Initialise wifi, message handlers and ir sender
*/
void setup() {
	Serial.begin(115200);
	wifiConnect(0);
	//Update service
	Serial.println(F("Set up Web update service"));
	MDNS.begin(host.c_str());
	httpUpdater.setup(&server, update_path, update_username, update_password);
	MDNS.addService("http", "tcp", AP_PORT);

	Serial.println(F("Set up Web command handlers"));
	server.on("/irjson" ,processIrjson);
	server.on("/ir", processIr);
	server.on("/macro", saveMacro);
	server.on("/check", checkStatus);
	server.on("/recent", recentCommands);
	server.on("/reload", reload);
	server.on("/list", HTTP_GET, handleFileList);
	//load editor
	server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");});
	//create file
	server.on("/edit", HTTP_PUT, handleFileCreate);
	//delete file
	server.on("/edit", HTTP_DELETE, handleFileDelete);
	//first callback is called after the request has ended with all parsed arguments
	//second callback handles file uploads at that location
	server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);

	//called when the url is not defined here
	//use it to load content from SPIFFS
	server.onNotFound([](){if(!handleFileRead(server.uri())) server.send(404, "text/plain", "FileNotFound");});
	server.begin();
	Serial.println("Set up IR sender");
	bitTx_init(irPin, irFrequency, IR_TIMER);
	Serial.println(F("Set up filing system"));
	initFS();
	if(alexaPin >=0) {
		pinMode(alexaPin, INPUT_PULLUP);
	}
	Serial.println(bitMessages_init());
	Serial.println(F("Set up complete"));
}

void loop() {
	server.handleClient();
	if (alexaPin >= 0) checkAlexa();
	delaymSec(timeInterval);
	elapsedTime++;
	wifiConnect(1);
}
