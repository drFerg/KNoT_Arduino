import serial, sys
from time import gmtime, strftime

if sys.argv < 3:
	print "Please specify serial port and baud to log"
	sys.exit()

f = open("output.csv","wr")
gateway = serial.Serial(sys.argv[1], sys.argv[2])

while True:
	line = gateway.readline()
	print line,
	if "Bedroom:" in line:
		f.write(strftime("%d/%m/%Y %H:%M:%S,", gmtime()))
		f.write(line[8:-1])
		f.write("\n")
		print line[9:-1]
