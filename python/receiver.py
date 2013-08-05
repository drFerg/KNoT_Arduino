import serial, sys
from time import gmtime, strftime
from protocolwrapper import (
    ProtocolWrapper, ProtocolStatus)
from knot_packet import (
    payload_header, data_header, 
    data_payload, serial_query, query_response, Container)

PROTOCOL_HEADER = '\x12'
PROTOCOL_FOOTER = '\x13'
PROTOCOL_DLE = '\x7D'


if sys.argv < 3:
	print "Please specify serial port and baud to log"
	sys.exit()


pw = ProtocolWrapper(   
        header=PROTOCOL_HEADER,
        footer=PROTOCOL_FOOTER,
        dle=PROTOCOL_DLE)


f = open(strftime("%d%m%Y-%H%M%S", gmtime())+ "output.csv","wr")
gateway = serial.Serial(sys.argv[1], sys.argv[2], timeout=100)

while True:
	status = pw.input(gateway.read(1))
	if (status == ProtocolStatus.MSG_OK):
		print "packet found:"
		dp = data_payload.parse(pw.last_message)
		print dp
		if (dp.cmd == 2):
			qr = query_response.parse(bytearray(dp.data))
			print qr