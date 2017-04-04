# IRBlasterWeb
Infra Red remote control blaster using esp8266

Transmits remote control codes freceived from Web.
Built in simple web page mainly for testing. ip/ir
Normal use is via POST messages. ip/irjson
Status check at ip/check
Update OTA to new binary firmare at ip/firmware

Command arguments
  auth (pincode or password to match built in value)
  device (name of remote control)
  parameter (name of button on remote control)
    If %is 1st char then the following hex code is used rather than looking up in device config
  repeat (number of times to send ir code)
  wait (mSec delay after sending code)
  
JSON version is an array of commands allowing a sequence to be used.

Config
  Edit IRBlasterWeb.ino
    AP_SSID Local network ssid
	AP_PASSWORD 
	AP_PORT
	AP_IP If static IP to be used
	AP_AUTHID Pincode or password to authorise web commands
	update_username user for updating firmware
	update_password
	
Remote controls
  These are defined in BitDevices.h 
  Currently 3 devices are defined (Yamaha AV Box, LGTV, YouView(BT) )
  To add a new device
    Increase NUMBER_DEVICES
	Create an array of buttons and Hex codes for that button
	Append 9 lines of new config data to devices e.g.
      	"youview", //name of device
	    "H9000,L4500", //header pulses
	    NULL, //trailer pulses
	    "H550,L550", // pulses for a data bit 0
	    "H550,L1600", // pulses for a data bit 1
	    100, //repeat gap mSec
	    33, //bit count
	    1, // minimum repeat
	    youviewMsgs  // pointer to button array
		
Libraries
  BitMessages Routines to look up and create pulse sequences for a commands
  BitTx Bit bang routines to execute a pulse sequence
    Interrupt driven and supports accurate modulation
	

