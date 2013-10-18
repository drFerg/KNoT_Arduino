import serial, sys
from time import gmtime, strftime
import serial_wrapper as sw

if sys.argv < 3:
	print "Please specify serial port and baud to log"
	sys.exit()






gateway = serial.Serial(sys.argv[1], sys.argv[2], timeout=5)
msg = sw.build_message_to_send(0, 1, 0, 1);
while True:
	s = raw_input("Enter a command: ")
	args = s.split(" ")
	if (args[0] == 'q'):
		gateway.write(sw.build_query(1))
	elif (args[0] == 'c' and (len(args) == 3)): 
		print args
		gateway.write(sw.build_connect(int(args[1]), int(args[2])))