#include <Arduino.h>

#include <serialPrintf.h>
#include <string.h>
#include "knot_network.h"

#define DEBUG 1
#if DEBUG

#define PRINTF(...) serialPrintf(__VA_ARGS__)
#define PRINT(...) Serial.print(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


#define SEQNO_START 0
#define SEQNO_LIMIT 255


void increment_seq_no(ChannelState *state, DataPayload *dp){
   if (state->seqno >= SEQNO_LIMIT)
      state->seqno = SEQNO_START;
   else 
      state->seqno++;
   dp->hdr.seqno = state->seqno;
}

int check_seqno(ChannelState *state, DataPayload *dp){
   if (state->seqno > dp->hdr.seqno){
      PRINT("--Out of sequence--\n");
      Serial.print("--State SeqNo: ");Serial.print(state->seqno);
      Serial.print(" Pkt SeqNo: ");Serial.print(dp->hdr.seqno);
      Serial.print("--\n--Dropping packet--\n");
      return 0;
   }
   else {
      state->seqno = dp->hdr.seqno;
      Serial.print("--SeqNo ");Serial.print(dp->hdr.seqno);Serial.print("--\n");
      if (state->seqno >= SEQNO_LIMIT){
         state->seqno = SEQNO_START;
         Serial.print("--Reset SeqNo\n");
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
void copy_link_address(ChannelState *state){
   copy_address(state);
}

void clean_packet(DataPayload *dp){
   memset(dp, '\0', sizeof(DataPayload));
}

void ping(ChannelState *state){
   DataPayload *new_dp = &(state->packet);
   clean_packet(new_dp);   
   new_dp->hdr.src_chan_num = state->chan_num;
   new_dp->hdr.dst_chan_num = state->remote_chan_num;
   (new_dp)->hdr.cmd = PING;
   (new_dp)->dhdr.tlen = 0;
   send_on_knot_channel(state, new_dp);
   state->state = STATE_PING;
   state->ticks = 100;
}

void pack_handler(ChannelState *state, DataPayload *dp){
   if (state->state != STATE_PING) {
      Serial.print("Not in PING state\n");
      return;
   }
   state->state = STATE_CONNECTED;
   state->ticks = 100;
   state->pingOUT = 0;

}

void ping_handler(ChannelState *state,DataPayload *dp){
   if (state->state != STATE_CONNECTED) {
      PRINT("Not in Connected state\n");
      return;
   }

   PRINT("PINGing back\n");
   DataPayload *new_dp = &(state->packet);
   clean_packet(new_dp);
   new_dp->hdr.src_chan_num = state->chan_num;
   new_dp->hdr.dst_chan_num = state->remote_chan_num;
   (new_dp)->hdr.cmd = PACK; 
   (new_dp)->dhdr.tlen = 0;
   send_on_knot_channel(state,new_dp);
   state->ticks = 100;
}


void close_graceful(ChannelState *state){
   DataPayload *new_dp = &(state->packet);
   clean_packet(new_dp);
   new_dp->hdr.src_chan_num = state->chan_num;
   new_dp->hdr.dst_chan_num = state->remote_chan_num;
   (new_dp)->hdr.cmd = DISCONNECT;
   (new_dp)->dhdr.tlen = 0;
   send_on_knot_channel(state,new_dp);
   state->state = STATE_DCONNECTED;
   state->ticks = 100;
}

void close_handler(ChannelState *state, DataPayload *dp){
   PRINT("Sending CLOSE ACK...\n");
   DataPayload *new_dp = &(state->packet);
   clean_packet(new_dp);
   new_dp->hdr.src_chan_num = dp->hdr.dst_chan_num;
   new_dp->hdr.dst_chan_num = dp->hdr.src_chan_num;
   (new_dp)->hdr.cmd = DACK;
   (new_dp)->dhdr.tlen = 0;
   send_on_knot_channel(state,new_dp);
}