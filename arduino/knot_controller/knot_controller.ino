#include <stdlib.h>
#include <string.h>

#include <EEPROM.h>
#include <cc1101.h>

#include "knot_protocol.h"
#include "payloads.h"
#include <TimerOne.h>
#include "knot_network_pan.h"
#include "knot_network.h"
#include "channeltable.h"
#include "LED.h"
#include "serialPacket.h"


#define SERIAL_SEARCH 1
#define SERIAL_CONNECT 2

#define PING_WAIT 3
#define TIMER_INTERVAL 3
#define HOME_CHANNEL 0

#define NETWORK_EVENT packetAvailable()
#define SERIAL_EVENT Serial.available()
#define TIMER_EVENT 0

char controller_name[] = "The Boss";
ChannelState home_channel_state;
int serial_ready = 0;
char buf[50];
int serial_index = 0;
int addr = 0;
char serialpkt[32];

void qack_handler(ChannelState *state, DataPayload *dp){
	if (state->state != STATE_QUERY) {
		Serial.print(F("Not in Query state\n"));
		return;
	}
	Serial.print(F("Query ACK received\n"));
	state->ticks = 100;
	QueryResponseMsg *qr = (QueryResponseMsg *)&dp->data;
	write_to_serial((char *)dp, sizeof(DataPayload));
	addr = state->remote_addr;
}

void cack_handler(ChannelState *state, DataPayload *dp){
	if (state->state != STATE_CONNECT){
		Serial.print(F("Not in Connecting state\n"));
		return;
	}
	ConnectACKMsg *ck = (ConnectACKMsg*)(dp->data);
	if (ck->accept == 0){
		Serial.print(F("SCREAM! THEY DIDN'T EXCEPT!!"));
		remove_channel(state->chan_num);
		return;
	}
	Serial.print(ck->name);
	Serial.print(F(" accepts connection request on channel "));
	Serial.println(dp->hdr.src_chan_num);
	state->remote_chan_num = dp->hdr.src_chan_num;

	DataPayload *new_dp = &(state->packet);
	clean_packet(new_dp);

	dp_complete(new_dp, state->chan_num, state->remote_chan_num, 
             CACK, NO_PAYLOAD);
	send_on_knot_channel(state,new_dp);
	state->state = STATE_CONNECTED;
	state->ticks = 100;
	//Set up ping timeouts for liveness if no message received or
	// connected to actuator
}

void response_handler(ChannelState *state, DataPayload *dp){
	if (state->state != STATE_CONNECTED && state->state != STATE_PING){
		Serial.print(F("Not connected to device!\n"));
		return;
	}
	state->ticks = 100;
	ResponseMsg *rmsg = (ResponseMsg *)dp->data;
	Serial.print(rmsg->name); Serial.print(": "); Serial.println(rmsg->data);
	/*RESET PING TIMER*/
}

void send_rack(ChannelState *state){
	// TODO: Reset internal timer/ticks of when to expect RSYN
	DataPayload *new_dp = &(state->packet);
	clean_packet(new_dp);
	dp_complete(new_dp, state->chan_num, state->remote_chan_num, 
             RACK, NO_PAYLOAD);
	send_on_knot_channel(state, new_dp);
}

void service_search(ChannelState* state, uint8_t type){
  DataPayload *new_dp = &(state->packet); 
  clean_packet(new_dp);
  dp_complete(new_dp, HOME_CHANNEL, HOME_CHANNEL, 
             QUERY, sizeof(QueryMsg));
  QueryMsg *q = (QueryMsg *) new_dp->data;
  q->type = type;
  strcpy(q->name, controller_name);
  knot_broadcast(state,new_dp);
  state->state = STATE_QUERY;
  state->ticks = 100;
  // Set timer to exit Query state after 5 secs~
}

void init_connection_to(ChannelState* state, uint8_t addr, int rate){
	ChannelState * s = new_channel();
	if (state == NULL) return;

	s->remote_addr = addr;
	s->rate = rate;
	DataPayload *new_dp = &(s->packet);
	clean_packet(new_dp);
	dp_complete(new_dp, s->chan_num, HOME_CHANNEL, 
             CONNECT, sizeof(ConnectMsg));
	ConnectMsg *cm = (ConnectMsg *)(new_dp->data);
	strcpy(cm->name, controller_name);
	cm->rate = rate;
    Serial.print(F("Sending connect request\n"));
    send_on_knot_channel(s, new_dp);
    s->state = STATE_CONNECT;
	s->ticks = 10;
}

void network_handler(){
	DataPayload dp;

	/* Gets data from the connection */
	uint8_t src = recv_pkt(&dp);
	if (src){
		Serial.print(F("KNoT>> Received packet from Thing: "));
		Serial.println(src);
	}
	else {
		return;//The cake was a lie
	}

	Serial.print(F("Data is "));Serial.print(dp.dhdr.tlen);
	Serial.print(F(" bytes long\n"));
	unsigned short cmd = dp.hdr.cmd;
	Serial.print(F("Received a "));Serial.print(cmdnames[cmd]);
	Serial.print(F(" command.\n"));
	Serial.print(F("Message for channel "));Serial.println(dp.hdr.dst_chan_num);
	
	ChannelState *state = NULL;

	/* Always allow disconnections to prevent crazies */
	if (cmd == DISCONNECT){
  		state = get_channel_state(dp.hdr.dst_chan_num);
		if (state){
			remove_channel(state->chan_num);
		}
		state = &home_channel_state;
		state->remote_addr = src;
  	} /* Special case for Homechannel which only responds to QACKs */
  	else if (dp.hdr.dst_chan_num == HOME_CHANNEL && cmd == QACK){
		state = &home_channel_state;
		state->remote_addr = src;
  	} /* The rest of the channels */
	else{
		state = get_channel_state(dp.hdr.dst_chan_num);
		if (state == NULL){
			Serial.print(F("Channel "));Serial.print(dp.hdr.dst_chan_num);Serial.print(F(" doesn't exist\n"));
			return;
		}
		if (check_seqno(state, &dp) == 0) {
			Serial.print(F("OH NOES\n"));
			return;
		} else { 
			//CHECK IF RIGHT CONNECTION	
		}
	}

	switch(cmd){
		case(QUERY):    	break;
		case(CONNECT): 	 	break;
		case(QACK):     	qack_handler(state, &dp);break;
		case(CACK):     	cack_handler(state, &dp);break;
		case(RESPONSE): 	response_handler(state, &dp);break;
		case(RSYN):		 	response_handler(state, &dp);send_rack(state);break;
		// case(CMDACK):   	command_ack_handler(state,dp);break;
		case(PING):     	ping_handler(state, &dp);break;
		case(PACK):     	pack_handler(state, &dp);break;
		case(DISCONNECT): 	close_handler(state,&dp);break;
		default: 			Serial.print(F("Unknown CMD type\n"));
	}

}

void serial_service_search(DataPayload *dp){
	service_search(&home_channel_state, ((SerialQuery*)dp)->type); 
}

void serial_init_connection_to(DataPayload *dp){
	SerialConnect *sc = (SerialConnect*)dp->data;
	Serial.print("Serial init conn to: ");Serial.println(dp->data[1]);
	init_connection_to(&home_channel_state, sc->addr, sc->rate);
}

void serial_handler(){
	DataPayload *dp = (DataPayload *)serialpkt;
	Serial.print(F("SERIAL> Serial command received.\n"));
	unsigned short cmd = dp->hdr.cmd;
	Serial.print("SERIAL> Packet length:");Serial.println(dp->dhdr.tlen);
	Serial.print(F("SERIAL> Message for channel "));Serial.println(dp->hdr.dst_chan_num);

	switch (cmd){
		case(SERIAL_SEARCH):  serial_service_search(dp);break;
		case(SERIAL_CONNECT): serial_init_connection_to(dp);break;
	}
}


void setup(){
	Serial.begin(38400);
	Serial.println(F(">> Controller initialising..."));
	randomSeed(analogRead(0));// Used for addr
	ledIOSetup();
	init_table();
	init_knot_network();
	set_dev_addr(random(1,256));
	blinker();
	home_channel_state.chan_num = 0;
	home_channel_state.remote_chan_num = 0;
	home_channel_state.state = STATE_IDLE;
	home_channel_state.remote_addr = 0;
	home_channel_state.rate = 60;
	Serial.println(F(">> Controller initialised!"));
	attach_serial(serial_handler, serialpkt);
	}

void loop(){
	if (NETWORK_EVENT)
		network_handler();
	else if (SERIAL_EVENT)
		recv_serial();
}
