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

# reverse these to change sense of signal, Active LOW
RX_DATA_OFF = GPIO.LOW
RX_DATA_ON = GPIO.HIGH
#RX_DATA_OFF = GPIO.HIGH
#RX_DATA_ON = GPIO.LOW

DUR = 10
def get_raw( check=True):

	print 'get-raw'
	d = []
	start_time = time.time()
	pin = GPIO.input(GPIO_RXDATA)
	print 'pin: ', pin
	i = 0
	t = start_time
	while ( time.time() - start_time) < DUR :
		while ( GPIO.input(GPIO_RXDATA) == pin ) and ( ( time.time() - start_time) < DUR ): 	# 
			pass
		t_last = t
		t = time.time()
		pin = GPIO.input(GPIO_RXDATA)
		i = i + 1
		a = int(round(1000000 * (t-t_last)))
		if pin == RX_DATA_ON :
			a = -1 * a
		d.append(a)
	j =0
	i = i -1
	print 'max:', i
	while ( i >= j ) :
		print d[j]
		j = j+1
	return 'Raw: ' + str(i), True
		

#def get_sirc( header1=2400, header2=600,  maxlow=800, maxhigh=1400, endgap=3000, check=True):
def get_sirc( header1=2500, header2=500,  maxlow=750, maxhigh=1500, minendgap=11000, maxendgap=23000, check=True):

#routine to decode sirc SONY type ir codes
# 12-bit version, 7 command bits, 5 address bits.
# 15-bit version, 7 command bits, 8 address bits.
# 20-bit version, 7 command bits, 5 address bits, 8 extended bits.
#	DATA bits, LSB to MSB order
#
# measured RM-AAP044 remote:
# code lengths alternate 15/20
# the gap varies, but total code length + gap = 41990
#
#  bits           19 +1 start bit
#  header       2430   565
#  one          1230   565
#  zero          628   565
#  gap          11380
#
#  bits			15+1 start bit
#  header		2540	460
#  one			1335	460
#  zero			735		460
#  gap			22055	
#
#
#
#lirc (SONY_RM-AAU014):
#  bits           15
#  header       2486   498
#  one          1291   502
#  zero          693   502
#  gap          40006
#  min_repeat      1
#  toggle_bit_mask 0x0
#
# standard:
# T = 	600us
# Start H4T, L1T
# 0: 	H1T, L1T
# 1: 	H2T, L1T
# 
#
# header1: 	exact start bit HIGH
# header2: 	exact start bit LOW
# maxlow: 	data bit maximum HIGH in case of bit 0, with some spare(200)
# maxhigh:	data bit maximum HIGH (1200), with some spare(200), bit 1 upper limit
# endgap:	minimum LOW at the end
	code = ''
	codehex=''
	startfound = False
	starttries = 0
	header1min = 0.8 * header1	# start bit HIGH min
	header1max = 1.2 * header1	# start bit HIGH max
	header2min = 0.8 * header2	# start bit LOW min
	header2max = 1.2 * header2	# start bit LOW max
	
	while not startfound and starttries < 10:
		while GPIO.input(GPIO_RXDATA) == RX_DATA_OFF:	# data LOW - no data
			pass
		t0 = time.time()
		while GPIO.input(GPIO_RXDATA) == RX_DATA_ON:	# data HIGH, Startbit 1. half
			pass
		t1 = time.time()	# 
		while GPIO.input(GPIO_RXDATA) == RX_DATA_OFF:	# data LOW, startbit 2nd half
			pass
		t2 = time.time()
		start1 = (t1-t0) * 1000000	# data HIGH
		start2 = (t2-t1) * 1000000	# data LOW
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
			pin = GPIO.input(GPIO_RXDATA)	# it should be data HIGH 1st time
			while GPIO.input(GPIO_RXDATA) == pin and (t0 - t2) < 1:	# wait for data change
				pass
			pulse = (time.time() - t0) * 1000000	# length of 1. bit data HIGH (from HIGH to LOW) at 1st time
			if pin == RX_DATA_ON and pulse < maxhigh:	# change from data HIGH to LOW
				if pulse < maxlow:	# HIGH for 0
					code = code + '0'
				else:
					code = code + '1'	# HIGH for 1
					nibblevalue = nibblevalue + bitvalue
				bitvalue = bitvalue / 2
				if bitvalue < 1:
					codehex = codehex + format(nibblevalue,'01X')
					bitvalue = 8
					nibblevalue = 0
			elif pin == RX_DATA_OFF and pulse > maxlow and pulse < maxhigh:	# change from data LOW to HIGH
																			# period is in between, can't decide, error
				code = code + '2'
				if check:
					return 'Bad data pulse:'+ str(int(round(pulse))) + ' pin: ' + str(pin) + ' place: ' + str(i) +' code:' + code, False
			elif pin == RX_DATA_OFF and pulse > minendgap and pulse < maxendgap:	# to cut the repetition, exit at gap
				if check and not ( len(code) == 12 or len(code) == 15 or len(code) == 20) :
					return 'Bad code length:' + str(len(code)) + " " + codehex + ',' + code, False
				if len(code)%4 != 0 :		# if code length is not dividable with 4, the last bits are lost
					codehex = codehex + format(nibblevalue,'01X')
				print 'Length:' + str(len(code)) + ' Code read: ' + codehex + ' , ' + code
				return str(len(code)) + ',' + codehex + ',' + code, True
		return 'Timeout, code length:' + str(len(code)) + " code:" + codehex + ',' + code, False
	else:
		return 'Bad start pulse ' + str(start1) + ' ' + str(start2), False
		
#routines to decode a NEC type ir code
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
	elif codetype == 'sirc':
		return get_sirc(check=chk)
	elif codetype == 'raw':
		return get_raw(check=chk)
	else:
		return 'unknown', False

def waitForQuiet(period=1):
	quiet = False
	hightime = time.time()
	while not quiet:
		pin = GPIO.input(GPIO_RXDATA)
		if pin == RX_DATA_OFF:
			if time.time() - hightime > period:
				quiet = True
		else:
			hightime = time.time()
		
# Main routine
# Set pin for input
GPIO.setup(GPIO_RXDATA,GPIO.IN)  #
remotename = raw_input('Name of remote control :')
buttons = raw_input('Name of subset buttons :')
codetype = raw_input('Codetype (nec,nec1,rc5,rc6,sirc,raw) :')
check = raw_input('Retry on bad code (y/n) :')

with open(remotename + '-' + buttons) as f:
    clines = f.read().splitlines() 
codefile = open(remotename + CODE_EXT, "a")
codefile.write('IR codes for ' + remotename + '-' + buttons + '\n')
print "codefile open"
try:
	for cline in clines:
		retries = 4
		print "cline: ",cline
		while retries > 0:
			print "Remote button ",cline," Waiting for quiet", waitForQuiet()
			print "Press Now ",cline
			result = get_ir(codetype, check == 'y')
			codefile.write(cline + ',' + result[0] + '\n')
			if check != 'y' or result[1]:
				retries = 0
			else:
				print "Error. Try again", result[0],result[1]
				retries = retries - 1
except KeyboardInterrupt:
	pass
	
codefile.close()

GPIO.cleanup()
