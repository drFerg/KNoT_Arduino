#include <stdlib.h>
#include <string.h>

#include "EEPROM.h"
#include "cc1101.h"

#include <knot_protocol.h>

#include <knot_network_pan.h>
#include <knot_network.h>
#include <channeltable.h>
#include <serialPrintf.h>
#include "panstamp.h"
ChannelState home_channel_state;
#define LEDONOFF 3
#define LEDOUTPUT 4
#define DEVICE_ADDRESS 5


void init_home_channel(){

  home_channel_state.chan_num = 0;
  home_channel_state.remote_chan_num = 0;
  home_channel_state.seqno = 0;
  home_channel_state.remote_addr = 1;


}
void service_search(ChannelState* state, uint8_t type){

  DataPayload *new_dp = &(state->packet); clean_packet(new_dp);
  //dp_complete(new_dp,10,QACK,1); new_dp->hdr.src_chan_num = state->chan_num;
  new_dp->hdr.dst_chan_num = 0; (new_dp)->hdr.cmd = QUERY;
  (new_dp)->dhdr.tlen = sizeof(QueryMsg); QueryMsg *q = (QueryMsg *)
  new_dp->data;

  q->type = type;
  strcpy(q->name, "BLARH");
  knot_broadcast(state,new_dp);
  state->state = STATE_QUERY;
  state->ticks = 100;
}

void blinker(){
  digitalWrite(LEDOUTPUT, HIGH);
  delay(500);
  digitalWrite(LEDOUTPUT, LOW);
  delay(100);
}

void setup(){
	
  Serial.begin(38400);
  Serial.println("start");

  pinMode(LEDOUTPUT, OUTPUT);
  digitalWrite(LEDOUTPUT, LOW);
  
  pinMode(LEDONOFF, OUTPUT);
  digitalWrite(LEDONOFF, HIGH);
  Serial.println("device initialized");

  Serial.println("setup done");
  init_knot_network();
  set_dev_addr(DEVICE_ADDRESS);
  init_home_channel();
  blinker();
}

void loop(){
  DataPayload dp;
  char buf[CC1101_DATA_LEN];
  ping(&home_channel_state);
  home_channel_state.state = STATE_CONNECTED;
  ping_handler(&home_channel_state, &(home_channel_state.packet));
  delay(500);
  // Serial.print("Ping...\n");
  // delay(1000);
  // if (recv_pkt(&dp)){
  //   Serial.print("Pong!!!\n");
  //   Serial.print(dp.hdr.cmd);
  // }
  // else {
  //   Serial.print("Poof\n");
  // }
  // delay(4000);

}