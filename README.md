# IRBlasterWeb
## Infra Red remote control blaster using esp8266

### Changes 2019/01/15
#### temperature code excluded if not required
#### merged with iLumos repository 
#### extra definition files added

### Features
- Transmits remote control codes freceived from Web.
- Remote control set up is in SPIFFs files which allows new set ups without changing code
- Files can be updated and deleted and viewed via web page. ip/edit or ip/edit?file=filename to view
- Built in simple web page mainly for testing. ip
- Normal use is via POST messages. ip/irjson
- Status check at ip/check (also returns list of macros)
- Update OTA to new binary firmare at ip/firmware
- Macro facility using files stored on SPIFFS
- Log recent commands ip/recent
- Incorporates WifiManager library for initial wifi set up
- Supports an Amazon Echo/Dot activate detector to mute / quieten as soon as activate word is spoken.
- includes temperature sensing and reporting to easyIOT (optional)

### Commands are sent to ip/ir with arguments
- auth (pincode or password to match built in value)
- device
	- name of remote control,
		- 'null' if just wait required,
		- 'macro' to use a macro from SPIFFS
		- 'detect' to turn alexa detect on / off
	- parameter
		- button name on remote control,
		- macro name,
		- 0/1 for detect on off
      
	- repeat (number of times to send ir code)
	- wait (mSec delay after sending code)
	- bits 0 for default, non zero overrides device definition
  
parameter is normally the name of the button on the control and the code to use is found by looking up in the buttonnames list and then finding the code for this button in the device set up.
If % is the first char of the parameter then the following code is used rather than by looking it up.
Code definitions are normally just the hex bits to send. The definition may start with #bitcount# to override the default bit count for the device. This may be used in the device table or in supplied parameters. For example, 
%#20#12345 will send 20 bits from the hex string 12345. 	
If device is macro then parameter is the name of the file in SPIFFS containing the macro
  
JSON can be posted to /irjson and contains authorisation and an array of commands using the same arguments allowing a sequence to be used using the POST to irjson
Example
```
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
```

Macros are held in SPIFFS files with a .txt extension (e.g macro startall is in file /startall.txt). They have exactly the same content as a JSON POST. They are executed by using the device name 'macro'
and including the name of the macro file in the parameter field.

New macros can be generated by POSTING the macro content to /macro with the auth argument and an extra argument called macro containing name to be used.
Example
```
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
```

Existing macros can be removed by using the same procedure but with no commands content.

### Config
- Edit BaseConfig.h
	- WifiManager password
	- AP_PORT to access ir blaster web service
	- AP_AUTHID Pincode or password to authorise web commands
	- update password  for updating firmware
	- EIOT password if using temperature support
	- TEMPERATURE if temp sensing needed
	- Uncomment FASTCONNECT in BaseConfig.h as required
- On start up connect to WM_NAME network and browse to 192.168.4.1 to set up wifi
	
### Remote controls definitions
- buttonnames.txt is a file held in SPIFFs which just holds a list of global button names across all devices
- Individual remote controls are defined in dev_devicename files. Code allows for up to 6 devices but can be increased in bitMessages.h
- To define a device create a dev_devicename.txt file (look at supplied examples)
    - Config section holding base parameters of the remote
		- devicename e.g. lgtv 
		- H9000,L4500 pulse sequence at the start of each message
		- NULL pulse sequence at the end of each message
		- H550,L550 pulse sequence to represent a 0 bit
		- H550,L1600 pulse sequence to represent a 1 bit
		- 38000 ir modulation frequency
		- 0 used to do special encoding like in rc6
		- 100 gap in msec between repeat messages
		- 33 number of bits to transmit
		- 1 minimum number of repeats of message
		- 0 used to control toggling bits in rc codes
		- 20DF nec address which may be prepended to messages if not defined in the code. Leave empty if not nec protocol
		- -1 spare
	
	- Codes section consisting of lines of buttonname,hexcode
		- ONOFF,10EF	

### Other web commands
- /upload (simple one file uploader)
- /recent (lists recent activity)
- /check (shows basic status)
- /    (loads a web form to send commands manually)
- /edit (loads a web form to view file list and delete/ upload files)
- /edit?file=filename (view contents of a specific file)
- /reload (reloads buttonnames and device files. Use after changing any of these)

### Libraries
- BaseSupport library https://github.com/roberttidey/BaseSupport
- BitMessages Routines to look up and create pulse sequences for a commands
- BitTx Bit bang routines to execute a pulse sequence
	- Interrupt driven and supports accurate modulation
- WifiManager
- FS
- DNSServer
- ArduinoJson (must be v6 or greter)
- ESP8266mDNS
- ESP8266HTTPUpdateServer
- OneWire.h
- DallasTemperature.h

### Temperature sensing
If TEMPERATURE is defined to be 1 at the top of the sketch then temperature sensing support is included.
A config file espConfig is downloaded from a web host which contains some config data 
as described in the example file. The address of the EIOT server and its password need to be set up in the sketch.

### Install procedure
- Normal arduino esp8266 compile and upload
- A simple built in file uploader (/upload) should then be used to upload the 4 base files to SPIFF
  edit.htm.gz
  index.html
  favicon.ico
  graphs.js.gz
- The /edit operation is then supported to upload further blaster definition files
	
### Tool for gathering codes from a remote
This is a simple python program (rxir.py) expected to run on a Raspberry Pi and using a demodulated IR receiver connected to a GPIO.

Create a text file with a list of button names (broken into subsets for convenience). These files are named device-subset. When 
run it will prompt for device,subset,coding type (nec,rc5,rc6) and whether to check(y/n). The user is then asked to press the buttons. 

The codes are then appended to a file called device.ircodes. If check is y then it will try to verify code looks sensible and will retry if required.

Note that the program assumes the GPIO is low when IR is off. I use a simple 1 transistor buffer between the sensor to convert to 3.3V 
which inverts the signal from the sensor (active low). If no inversion is used then modify the defintions at line 26-27
  
### Triggering from Alexa
- This area has changed due to IFTTT being removed from Alexa integration and changes in Alexa skills development.
- Set up Router to port forward external requests to IRBlaster device. Use a dns service if possible to give router external IP a name
- Create a login for developer.amazon.com if you don't have one. Use same amazon login as your main Amazon login / alexa devices
- Create a new custom skill in the Alexa skills IDE and upload the skill definition from the Alexa folder included with this repository.
- Build the skill model in the IDE.
- On the code tab edit the lambda_function.py file. Edit the commands dictionary to add or remove button names and the associated json to send to the ir blaster
- edit the url in the transmitIntent to have the server address and port for Alexa to send the commands to. The %c is replaced by the selected command when invoked. `
- save and deploy the skill. It should be available under your skills/dev in the Alexa App. Make sure it is enabled.
- You can now say something like "Alexa Ask remote control to push power" to send the power command.
  
### Alexa activate detector (Optional)
- An input pin can be used to interface to an Alexa activate detector.
- This uses a light dependent resistor to detect when the LED rings on the Dot light up.
- This can be used to send out ir commands locally to mute the sound and make the recognition of the Alexa command more reliable.
- When detected the software tries to run an alexaon or alexaoff macro. See http://www.thingiverse.com/thing:2305400
- The detect command can be useful to turn this on and off. E.g. only turn it on when using a macro to turn all equipment on and disable it
when turning equipment off.
- Also can be used in handling mute on and off. E.g. turning detect off leaves it in a muted state. Similarly turning it on can
be used to re-enable sound.
	
  
	

