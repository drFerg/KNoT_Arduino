/*
* author Fergus William Leahy
*/
#ifndef KNOT_CHANNEL_STATE_H
#define KNOT_CHANNEL_STATE_H

#include "knot_protocol.h"


typedef struct channel_state{
   //CallbackControlBlock ccb;
   uint8_t remote_addr; //Holds address of remote device
   uint8_t state;
   uint8_t seqno;
   uint8_t chan_num;
   uint8_t remote_chan_num;
   uint16_t ticks;
   uint16_t rate;
   uint8_t pingOUT;
   uint8_t timer;
   DataPayload packet;
}ChannelState;


void init_state(ChannelState *state, uint8_t chan_num);


#endif /* KNOT_CHANNEL_STATE_H */