/* KNoT Protocol definition
*
* Hardware and link/transport level independent protocol definition
*
* author Fergus William Leahy
*/ 
#ifndef KNOT_PROTOCOL_H
#define KNOT_PROTOCOL_H
#include <stdint.h>
//Sensor type
#define TEMP   1
#define HUM    2
#define SWITCH 3
#define LIGHT  4

/* Connection states */
#define STATE_IDLE       0
#define STATE_QUERY      1
#define STATE_QACKED     2
#define STATE_CONNECT    3
#define STATE_CONNECTED  4
#define STATE_DCONNECTED 5
#define STATE_PING       7
#define STATE_COMMANDED  9
/* ===================*/



/* Packet command types */
#define QUERY    1
#define QACK     2
#define CONNECT  3
#define CACK     4
#define RESPONSE 5
#define RACK     6
#define DISCONNECT 7
#define DACK     8
#define CMD      9
#define CMDACK  10
#define PING    11
#define PACK    12
#define SEQNO   13
#define SACK    14


#define CMD_LOW CONNECT
#define CMD_HIGH SACK		/* change this if commands added */

/* =======================*/

#define MAX_DATA_SIZE 32
#define RESPONSE_DATA_SIZE 16
#define NAME_SIZE     16

extern char *cmdnames[15];

typedef struct ph {
   uint8_t seqno;   /* sequence number */
   uint8_t src_chan_num;
   uint8_t dst_chan_num;
   uint8_t cmd;	/* message type */
   uint16_t chksum;
} PayloadHeader;

typedef struct dh {
   uint16_t tlen;	/* total length of the data */
} DataHeader;


typedef struct dp {		/* template for data payload */
   PayloadHeader hdr;
   DataHeader dhdr;
   unsigned char data[MAX_DATA_SIZE];	/* data is address of `len' bytes */
} DataPayload;




/* Message Payloads */

typedef struct query{
   uint8_t type; 
   char name[NAME_SIZE];
}QueryMsg;

typedef struct query_response{
   uint8_t type;
   uint16_t rate;
   char name[NAME_SIZE];
}QueryResponseMsg;

typedef struct connect_message{
   uint16_t rate;
   char name[NAME_SIZE];
}ConnectMsg;

typedef struct cack{
   uint8_t accept;
   char name[NAME_SIZE];
}ConnectACKMsg;

typedef struct response{
   uint16_t data;
   char name[NAME_SIZE];
}ResponseMsg;

typedef struct data_response{
   uint8_t len;
   unsigned char data[MAX_DATA_SIZE-1];
}DataResponseMsg;





#endif /* KNOT_PROTOCOL_H */