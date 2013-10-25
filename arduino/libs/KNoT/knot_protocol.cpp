#include "knot_protocol.h"

const char *cmdnames[16] = {"", "QUERY", "QACK","CONNECT", "CACK", 
                                 "RESPONSE", "RACK", "DISCONNECT", "DACK",
                                 "COMMAND", "COMMANDACK", "PING", "PACK", "SEQNO",
                                 "SACK", "RSYN"};