#include "knot_protocol.h"

char *cmdnames[16] = {"", "QUERY", "QACK","CONNECT", "CACK", 
                                 "RESPONSE", "RACK", "DISCONNECT", "DACK",
                                 "COMMAND", "COMMANDACK", "PING", "PACK", "SEQNO",
                                 "SACK", "RSYN"};

void dp_complete(DataPayload *dp, uint8_t src, uint8_t dst, 
				 uint8_t cmd, uint8_t len){
	dp->hdr.src_chan_num = src; 
	dp->hdr.dst_chan_num = dst; 
    dp->hdr.cmd = cmd; 
    dp->dhdr.tlen = len;
}