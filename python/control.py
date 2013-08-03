import serial, sys
from time import gmtime, strftime
import serial_wrapper as sw

if sys.argv < 3:
	print "Please specify serial port and baud to log"
	sys.exit()

f = open("output.csv","wr")
gateway = serial.Serial(sys.argv[1], sys.argv[2], timeout=5)
msg = sw.build_message_to_send(0,1,0,1);
while True:
	s = raw_input("Enter a command: ")

	gateway.write(msg)
	print msg.encode('hex')


while True:
	print gateway.readline()



i = 0
while i < 5:
	i += 1
	print i, 
	buf = gateway.readline()

	if "[" in buf:
		print buf[1:-3].split(",")
	