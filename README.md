# IRBlasterWeb
Infra Red remote control blaster using esp8266

Transmits remote control codes freceived from Web.
Built in simple web page mainly for testing. ip/ir
Normal use is via POST messages. ip/irjson
Status check at ip/check (also returns list of macros)
Update OTA to new binary firmare at ip/firmware
Macro facility using files stored on SPIFFS
Log recent commands ip/recent
Incorporates WifiManager library for initial wifi set up
Supports an Amazon Echo/Dot activate detector to mute / quieten as soon as activate word is spoken.

Command arguments
  auth (pincode or password to match built in value)
  device
      name of remote control,
	  'null' if just wait required,
	  'macro' to use a macro from SPIFFS
	  'detect' to turn alexa detect on / off
  parameter
      button name on remote control,
	  macro name,
	  0/1 for detect on off
      
  repeat (number of times to send ir code)
  wait (mSec delay after sending code)
  bits 0 for default, non zero overrides device definition
  
  parameter is normally the name of the button on the control and the code to use is found in the device config.
    If % is the first char of the parameter then the following code is used rather than looking up in device config
	Code definitions are normally just the hex bits to send. The definition may start with #bitcount# to override the
    default bit count for the device. This may be used in the device table or in supplied parameters. For example,
    %#20#12345 will send 20 bits from the hex string 12345	
	If device is macro then parameter is the name of the file in SPIFFS containing the macro
  
JSON version is authorisation and an array of commands using the same arguments allowing a sequence to be used using the POST to irjson
	Example
{
	"auth":"1234",
	"commands": [
		{
			"device":"yamahaAV",
			"parameter":"hdmi4",
			"wait":"5000",
			"bits":"0",
			"repeat":"1"
		},
		{
			"device":"yamahaAV",
			"parameter":"hdmi1",
			"wait":"100",
			"bits":"0",
			"repeat":"1"
		}
	]
}

Macros are held in SPIFFS files and have exactly the same content as a JSON POST. They are executed by using the device name 'macro'
and including the name of the macro file in the parameter field.

New macros can be generated by POSTING the macro content to /macro with the auth argument and an extra argument called macro containing name to be used.
Example
{
	"auth":"1234",
	"macro":"test1",
	"commands": [
		{
			"device":"yamahaAV",
			"parameter":"hdmi4",
			"wait":"5000",
			"bits":"0",
			"repeat":"1"
		},
		{
			"device":"yamahaAV",
			"parameter":"hdmi1",
			"wait":"100",
			"bits":"0",
			"repeat":"1"
		}
	]
}

Existing macros can be removed by using the same procedure but with no commands content.

Config
  Edit IRBlasterWeb.ino
	Manual Wifi set up (Comment out WM_NAME)
      AP_SSID Local network ssid
	  AP_PASSWORD 
	  AP_IP If static IP to be used
	Wifi Manager set up
	  WM_NAME (ssid of wifi manager network)
	  WM_PASSWORD (password to connect)
	  Connect to WM_NAME and browse to 192.168.4.1 and set up
	AP_PORT to access ir blaster web service
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
		38000, //IR modulation frequency
		0, //special handling 0=Normal, 1 = rc6 bit 3 handling
	    100, //repeat gap mSec
	    33, //bit count
	    1, // minimum repeat
	    youviewMsgs  // pointer to button array
		
Libraries
  BitMessages Routines to look up and create pulse sequences for a commands
  BitTx Bit bang routines to execute a pulse sequence
    Interrupt driven and supports accurate modulation
  WifiManager
  FS
  DNSServer
  ArduinoJson
  ESP8266mDNS
  ESP8266HTTPUpdateServer
	
Install procedure
	Normal arduino esp8266 compile and upload
	As SPIFFS is used then the memory should be prepared by installing and using the ESP8266 Sketch Data upload tool
	This will upload the data folder as initial SPIFFS content
	
Tool for gathering codes from a remote
  This is a simple python program (rxir.py) expected to run on a Raspberry Pi and using a demodulated IR receiver connected to a GPIO.
  Create a text file with a list of button names (broken into subsets for convenience). These files are named device-subset. When
  run it will prompt for device,subset,coding type (nec,rc5,rc6) and whether to check(y/n). The user is then asked to press the buttons. 
  The codes are then appended to a file called device.ircodes. If check is y then it will try to verify code looks sensible and will retry if required.
  Note that the program assumes the GPIO is low when IR is off. I use a simple 1 transistor buffer between the sensor to convert to 3.3V
  which inverts the signal from the sensor (active low). If no inversion is used then modify the defintions at line 26-27
  
Triggering from Alexa / IFTTT
  Set up Router to port forward external requests to IRBlaster device. Use a dns service if possible to give router external IP a name
  Register with IFTTT and set up Alexa service using your Amazon login
  Create new Applet with Alexa as IF. Select trigger with phrase. Enter phrase to be the trigger, e.g tv on.
  Use Maker WebHooks as the THAT action.
  Enter a URL into Webhooks to invoke the IRBlaster action (typically use a macro) e.g
    http://yourExtIP:port?plain={"auth":"1234","commands":[{"device":"macro","parameter":"TVOn","wait":"1000"}]}
    Note that this needs to be in the IFTTT URL, putting the POST in the body in IFTTT does not seem to work
  Create and save the macro with same name (TVOn) in the IRBlaster to support the request
  
Alexa activate detector
  An input pin can be used to interface to an Alexa activate detector. This uses a light dependent resistor to detect when the LED rings on
  the Dot light up. This can be used to send out ir commands locally to mute the sound and make the recognition of the Alexa command more reliable.
  When detected the software tries to run an alexaon or alexaoff macro. See http://www.thingiverse.com/thing:2305400
  The detect command can be useful to turn this on and off. E.g. only turn it on when using a macro to turn all equipment on and disable it
  when turning equipment off. Also can be used in handling mute on and off. E.g. turning detect off leaves it in a muted state. Similarly turning it on can
  be used to re-enable sound.
	
  
	

