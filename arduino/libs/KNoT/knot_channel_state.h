/*
* author Fergus William Leahy
*/
#ifndef KNOT_CHANNEL_STATE_H
#define KNOT_CHANNEL_STATE_H

#include "knot_protocol.h"

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

#define TICKS_TILL_PING (60 * 50) /* 60s * (50ms * 20ms) = 1 minute */
#define PINGS_TILL_PURGE 3


typedef struct channel_state{
   //CallbackControlBlock ccb;
   uint8_t remote_addr; //Holds address of remote device
   uint8_t state;
   uint8_t seqno;
   uint8_t chan_num;
   uint8_t remote_chan_num;
   uint8_t ticks;
   uint8_t ticksLeft;
   uint16_t rate;
   uint8_t pingsTillPurge;
   uint8_t ticksTillPing;
   uint8_t timer;
   DataPayload packet;
}ChannelState;


void init_state(ChannelState *state, uint8_t chan_num);


#endif /* KNOT_CHANNEL_STATE_H */