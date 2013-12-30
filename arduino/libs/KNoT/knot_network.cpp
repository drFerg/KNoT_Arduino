#include <Arduino.h>

#include <string.h>
#include "knot_network.h"

#define DEBUG 1
#if DEBUG
#define PRINT(...) Serial.print(__VA_ARGS__)
#endif

#define SEQNO_START 0
#define SEQNO_LIMIT 255

void dp_complete(DataPayload *dp, uint8_t src, uint8_t dst, 
             uint8_t cmd, uint8_t len){
   dp->hdr.src_chan_num = src; 
   dp->hdr.dst_chan_num = dst; 
   dp->hdr.cmd = cmd; 
   dp->dhdr.tlen = len;
}

void increment_seq_no(ChannelState *state, DataPayload *dp){
   if (state->seqno >= SEQNO_LIMIT){
      state->seqno = SEQNO_START;
   } else {
      state->seqno++;
   }
   dp->hdr.seqno = state->seqno;
}

int valid_seqno(ChannelState *state, DataPayload *dp){
   if (state->seqno > dp->hdr.seqno){ // Old packet or sender confused
      return 0;
   } else {
      state->seqno = dp->hdr.seqno;
      if (state->seqno >= SEQNO_LIMIT){
         state->seqno = SEQNO_START;
      }
      return 1;
   }
}

int init_knot_network(){
   init_link_layer();
   return 1;
}

void send_on_knot_channel(ChannelState *state, DataPayload *dp){
   increment_seq_no(state, dp);
   send_on_channel(state, dp);
}

void knot_broadcast(ChannelState *state, DataPayload *dp){
   increment_seq_no(state, dp);
   broadcast(state,dp);
}

void set_broadcast(Address a){
   copy_address_broad(a);
}

void copy_link_address(Address a, Address b){
   copy_address(a, b);
}

void ping(ChannelState *state){
   DataPayload *new_dp = &(state->packet);
   clean_packet(new_dp);
   dp_complete(new_dp, state->chan_num, state->remote_chan_num, 
               PING, NO_PAYLOAD);
   send_on_knot_channel(state, new_dp);
   state->state = STATE_PING;
}

void pack_handler(ChannelState *state, DataPayload *dp){
   if (state->state != STATE_PING) {
      Serial.print(F("Not in PING state\n"));
      return;
   }
   state->state = STATE_CONNECTED;

}

void ping_handler(ChannelState *state, DataPayload *dp){
   if (state->state != STATE_CONNECTED) {
      Serial.print(F("Not in Connected state\n"));
      return;
   }

   Serial.print(F("PINGing back\n"));
   DataPayload *new_dp = &(state->packet);
   clean_packet(new_dp);
   dp_complete(new_dp, state->chan_num, state->remote_chan_num, 
               PACK, NO_PAYLOAD);
   send_on_knot_channel(state,new_dp);
}


void close_graceful(ChannelState *state){
   DataPayload *new_dp = &(state->packet);
   clean_packet(new_dp);
   dp_complete(new_dp, state->chan_num, state->remote_chan_num, 
               DISCONNECT, NO_PAYLOAD);
   send_on_knot_channel(state,new_dp);
   state->state = STATE_DCONNECTED;
}

void close_handler(ChannelState *state, DataPayload *dp){
   Serial.print(F("Sending CLOSE ACK...\n"));
   DataPayload *new_dp = &(state->packet);
   clean_packet(new_dp);
   dp_complete(new_dp, state->chan_num, state->remote_chan_num, 
               DACK, NO_PAYLOAD);
   send_on_knot_channel(state, new_dp);
}