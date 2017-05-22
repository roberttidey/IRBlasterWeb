#!/usr/bin/python
# rxir.py
# captures codes from ir remotes
# uses a control file to manage the collection of data
#
# Author : Bob Tidey
# Date   : 11/02/2017
import time
import RPi.GPIO as GPIO
import array

# -----------------------
# Main Script
# -----------------------
MAX_BUFFER = 40000 # twice number of samples per file
MAX_FILES = 2
CODE_EXT = ".ircodes"

# Use BCM GPIO references
# instead of physical pin numbers
GPIO.setmode(GPIO.BCM)
# Define GPIO to use on Pi
GPIO_RXDATA = 24

# reverse these to change sense of signal
RX_DATA_OFF = GPIO.LOW
RX_DATA_ON = GPIO.HIGH

#routines to decode an nec type ir code
def get_nec(header1=9000, header2=5000, maxlow=800, maxhigh=2000, endgap=3000, check=True):
	code = ''
	codehex=''
	startfound = False
	starttries = 0
	header1min = 0.6 * header1
	header1max = 1.5 * header1
	header2min = 0.6 * header2
	header2max = 1.5 * header2
	while not startfound and starttries < 10:
		while GPIO.input(GPIO_RXDATA) == RX_DATA_OFF:
			pass
		t0 = time.time()
		while GPIO.input(GPIO_RXDATA) == RX_DATA_ON:
			pass
		t1 = time.time()
		while GPIO.input(GPIO_RXDATA) == RX_DATA_OFF:
			pass
		t2 = time.time()
		start1 = (t1-t0) * 1000000
		start2 = (t2-t1) * 1000000
		if start1 > header1min and start1 < header1max and start2 > header2min and start2 < header2max:
			startfound = True
		else:
			starttries = starttries + 1
	if startfound:
		t2 = time.time()
		bitvalue = 8
		nibblevalue = 0
		while time.time() - t2 < 5:
			t0 = time.time()
			pin = GPIO.input(GPIO_RXDATA)
			while GPIO.input(GPIO_RXDATA) == pin and (t0 - t2) < 1:
				pass
			pulse = (time.time() - t0) * 1000000
			if pin == RX_DATA_OFF and pulse < maxhigh:
				if pulse < maxlow:
					code = code + '0'
				else:
					code = code + '1'
					nibblevalue = nibblevalue + bitvalue
				bitvalue = bitvalue / 2
				if bitvalue < 1:
					codehex = codehex + format(nibblevalue,'01X')
					bitvalue = 8
					nibblevalue = 0
			elif pin == RX_DATA_ON and pulse > maxlow and pulse < maxhigh:
				code = code + '2'
				if check:
					return 'Bad data pulse', False
			elif pulse < endgap:
				pass
			else:
				if check and len(code) != 32:
					return 'Bad code length' + codehex + ',' + code, False
				return codehex + ',' + code, True
		return 'Timeout', False
	else:
		return 'Bad start pulse ' + str(start1) + ' ' + str(start2), False

def get_rc5(midpulse=1200, endgap=3000, check=True):
	code = ''
	codehex=''
	state = 1
	while GPIO.input(GPIO_RXDATA) == RX_DATA_OFF:
		pass
	t0 = time.time()
	level = RX_DATA_ON
	incode = True
	bitvalue = 8
	nibblevalue = 0
	event = 0
	while incode:
		emit = ''
		while GPIO.input(GPIO_RXDATA) == level:
			pass
		event = event + 1
		t1 = time.time()
		td = (t1 - t0) * 1000000
		t0 = t1
		level = GPIO.input(GPIO_RXDATA)
		if td < endgap:
			if state == 0 and level == RX_DATA_ON and td < midpulse:
				emit = '1'
				state = 1
			elif state == 1 and level == RX_DATA_OFF and td < midpulse:
				state = 0
			elif state == 1 and level == RX_DATA_OFF:
				emit = '0'
				state = 2
			elif state == 2 and level == RX_DATA_ON and td < midpulse:
				state = 3
			elif state == 2 and level == RX_DATA_ON:
				emit = '1'
				state = 1
			elif state == 3 and level == RX_DATA_OFF and td < 1200:
				emit = '0'
				state = 2
			else:
				return code + 'bad level:event:state:time ' + str(event) + str(level) + ':' + str(state) + ':' + str(td), False
			if emit != '':
				code = code + emit
				if emit == '1':
					nibblevalue = nibblevalue + bitvalue
				bitvalue = bitvalue / 2
				if bitvalue < 1:
					codehex = codehex + format(nibblevalue,'01X')
					bitvalue = 8
					nibblevalue = 0
		else:
			incode = False
	if bitvalue != 8:
		codehex = codehex + format(nibblevalue,'01X')
	if check and len(code) != 13:
		return 'Bad code length', False
	return codehex + ',' + code, True

def find_rc6header(leader=2200, pulse=450):
	starttries = 0
	leadermin = round(0.5 * leader)
	leadermax = round(2.0 * leader)
	pulsemin = round(0.5 * pulse)
	pulsemax = round(2.0 * pulse)
	while starttries < 1:
		waitForQuiet(0.5)
		while GPIO.input(GPIO_RXDATA) == RX_DATA_OFF:
			pass
		t0 = time.time()
		while GPIO.input(GPIO_RXDATA) == RX_DATA_ON:
			pass
		t1 = time.time()
		while GPIO.input(GPIO_RXDATA) == RX_DATA_OFF:
			pass
		t2 = time.time()
		while GPIO.input(GPIO_RXDATA) == RX_DATA_ON:
			pass
		t3 = time.time()
		pulse0 = round((t1-t0) * 1000000)
		pulse1 = round((t2-t1) * 1000000)
		pulse2 = round((t3-t2) * 1000000)
		if (pulse0 > leadermin) and (pulse0 < leadermax) and (pulse1 > pulsemin) and (pulse1 < pulsemax) and (pulse2 > pulsemin) and (pulse2 < pulsemax):
			return ""
		else:
			starttries = starttries + 1
	return 'No header ' + str(pulse0) + ' ' + str(pulse1) + ' ' + str(pulse2)
	
def get_rc6(leader=2200, pulse=450, endgap=3000, check=True):
	midpulse = 1.42 * pulse
	header = find_rc6header(leader, pulse)
	if header != "":
		return header, False
	code = ''
	codehex=''
	state = 1
	while GPIO.input(GPIO_RXDATA) == RX_DATA_OFF:
		pass
	t0 = time.time()
	level = RX_DATA_ON
	incode = True
	bitcount = 0
	bitvalue = 8
	nibblevalue = 0
	event = 0
	while incode:
		emit = ''
		while GPIO.input(GPIO_RXDATA) == level:
			pass
		event = event + 1
		t1 = time.time()
		td = (t1 - t0) * 1000000
		t0 = t1
		if bitcount == 2:
			td = td * 0.5
		level = GPIO.input(GPIO_RXDATA)
		if td < endgap:
			if state == 0 and level == RX_DATA_ON and td < midpulse:
				emit = '1'
				state = 1
			elif state == 1 and level == RX_DATA_OFF and td < midpulse:
				state = 0
			elif state == 1 and level == RX_DATA_OFF:
				emit = '0'
				state = 2
			elif state == 2 and level == RX_DATA_ON and td < midpulse:
				state = 3
			elif state == 2 and level == RX_DATA_ON:
				emit = '1'
				state = 1
			elif state == 3 and level == RX_DATA_OFF and td < 1200:
				emit = '0'
				state = 2
			else:
				return code + 'bad level:event:state:time ' + str(event) + str(level) + ':' + str(state) + ':' + str(td), False
			if emit != '':
				bitcount = bitcount + 1
				code = code + emit
				if emit == '1':
					nibblevalue = nibblevalue + bitvalue
				bitvalue = bitvalue / 2
				if bitvalue < 1:
					codehex = codehex + format(nibblevalue,'01X')
					bitvalue = 8
					nibblevalue = 0
		else:
			incode = False
	if bitvalue != 8:
		codehex = codehex + format(nibblevalue,'01X')
	if check and len(code) != 13:
		return 'Bad code length', False
	return codehex + ',' + code

def get_ir(codetype, chk):
	if codetype == 'nec':
		return get_nec(check=chk)
	elif codetype == 'nec1':
		return get_nec(header1=4500, check=chk)
	elif codetype == 'rc5':
		return get_rc5(check=chk)
	elif codetype == 'rc6':
		return get_rc6(check=chk)
	else:
		return 'unknown', False

def waitForQuiet(period=1):
	quiet = False
	hightime = time.time()
	while not quiet:
		pin = GPIO.input(GPIO_RXDATA)
		if pin == GPIO.LOW:
			if time.time() - hightime > period:
				quiet = True
		else:
			hightime = time.time()
		
# Main routine
# Set pin for input
GPIO.setup(GPIO_RXDATA,GPIO.IN)  #
remotename = raw_input('name of remote control :')
buttons = raw_input('name of subset buttons :')
codetype = raw_input('codetype (nec,nec1,rc5,rc6) :')
check = raw_input('retry on bad code (y/n) :')
with open(remotename + '-' + buttons) as f:
    clines = f.read().splitlines() 
codefile = open(remotename + CODE_EXT, "a")
codefile.write('IR codes for ' + remotename + '-' + buttons + '\n')

try:
	for cline in clines:
		retries = 4
		while retries > 0:
			print "Remote button ",cline," Waiting for quiet",
			waitForQuiet()
			print "Press Now ",cline
			result = get_ir(codetype, check == 'y')
			codefile.write(cline + ',' + result[0] + '\n')
			if check != 'y' or result[1]:
				retries = 0
			else:
				print "Error. Try again"
				retries = retries - 1
except KeyboardInterrupt:
	pass
	
codefile.close()

GPIO.cleanup()
