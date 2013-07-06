/*
* author Fergus William Leahy
*/
#include "knot_channel_state.h"
#include "knot_network_pan.h"

void init_state(ChannelState *state, uint8_t chan_num){
      state->chan_num = chan_num;
      state->seqno = 0;
      state->remote_addr = 0;
      state->pingOUT = 0;
}

