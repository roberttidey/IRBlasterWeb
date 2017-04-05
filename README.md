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
  parameter
    If %is 1st char then the following hex code is used rather than looking up in device config
  repeat (number of times to send ir code)
  wait (mSec delay after sending code)
  bits 0 for default, non zero overrides device definition
  
  parameter is normally the name of the button on the control and the code to use is found in the device config.
    If % is the first char of the parameter then the following code is used rather than looking up in device config
	Code definitions are normally just the hex bits to send. THe definition may start with #bitcount# to override the
    default bit count for the device. This may be used in the device table or in supplied parameters. For example,
    %#20#12345 will send 20 bits from the hex string 12345	
  
JSON version is an array of commands using the same arguments allowing a sequence to be used.

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
  These are defined in BitDevices.h in the BitMessages library
  Currently 3 devices are fully defined (Yamaha AV Box, LGTV, YouView(BT) )
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
	

