#include <stdlib.h>
#include <string.h>

#include <EEPROM.h>
#include <cc1101.h>
#include <TimerOne.h>
#include "knot_protocol.h"
#include "payloads.h"
#include "callback_timer.h"
#include "knot_network_pan.h"
#include "knot_network.h"
#include "channeltable.h"
#include "LED.h"
#include "serialPacket.h"
#define DEBUG 1

#if DEBUG
#define PRINT(...) Serial.print(__VA_ARGS__)
#define PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define PRINT(...)
#define PRINTLN(...)
#endif

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

void qack_handler(ChannelState *state, DataPayload *dp) {
	if (state->state != STATE_QUERY) {
		PRINT("Not in Query state\n");
		return;
	}
	PRINT("Query ACK received from Thing: \n");
	PRINTLN(state->remote_addr);
	SerialQueryResponseMsg *qr = (SerialQueryResponseMsg *) &dp->data;
	qr->src = state->remote_addr;
	PRINTLN(qr->src);
	write_to_serial((char *)dp, sizeof(DataPayload));
}

void cack_handler(ChannelState *state, DataPayload *dp){
	if (state->state != STATE_CONNECT){
		PRINT("Not in Connecting state\n");
		return;
	}
	ConnectACKMsg *ck = (ConnectACKMsg*)(dp->data);
	if (ck->accept == 0){
		PRINT("SCREAM! THEY DIDN'T EXCEPT!!");
		remove_channel(state->chan_num);
		return;
	}
	PRINT(ck->name);PRINT(" accepts connection request on channel ");
	PRINTLN(dp->hdr.src_chan_num);
	state->remote_chan_num = dp->hdr.src_chan_num;

	DataPayload *new_dp = &(state->packet);
	clean_packet(new_dp);

	dp_complete(new_dp, state->chan_num, state->remote_chan_num, 
             CACK, NO_PAYLOAD);
	send_on_knot_channel(state,new_dp);
	set_ticks(state, TICKS);
	set_state(state, STATE_CONNECTED);
	//Set up ping timeouts for liveness if no message received or
	// connected to actuator
	SerialConnectACKMsg *sck = (SerialConnectACKMsg *) ck;
	sck->src = state->remote_addr;
	write_to_serial((char *)dp, sizeof(DataPayload));
}

void response_handler(ChannelState *state, DataPayload *dp){
	if (state->state != STATE_CONNECTED && state->state != STATE_PING){
		PRINT("Not connected to device!\n");
		return;
	}
	set_ticks(state, TICKS); /* RESET PING TIMER */
	ResponseMsg *rmsg = (ResponseMsg *)dp->data;
	PRINT(rmsg->name); PRINT(": "); PRINTLN(rmsg->data);
	SerialResponseMsg *srmsg = (SerialResponseMsg *)dp->data;
	srmsg->src = state->remote_addr;
	write_to_serial((char *)dp, sizeof(DataPayload));
	
}

void send_rack(ChannelState *state){
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
    knot_broadcast(state, new_dp);
    set_ticks(state, TICKS);
    set_state(state, STATE_QUERY);
    // Set timer to exit Query state after 5 secs~
}

void init_connection_to(uint8_t addr, int rate){
	ChannelState * new_state = new_channel();
	if (new_state == NULL) return;
	new_state->remote_addr = addr;
	new_state->rate = rate;
	DataPayload *new_dp = &(new_state->packet);
	clean_packet(new_dp);
	dp_complete(new_dp, new_state->chan_num, HOME_CHANNEL, 
             CONNECT, sizeof(ConnectMsg));
	ConnectMsg *cm = (ConnectMsg *)(new_dp->data);
	strcpy(cm->name, controller_name);
	cm->rate = rate;
    PRINT("Sending connect request\n");
    send_on_knot_channel(new_state, new_dp);
    set_ticks(new_state, TICKS);
    set_state(new_state, STATE_CONNECT);
	
}

/* Checks the timer for a channel's state, retransmitting when necessary */
void check_timer(ChannelState *s) {
    if (s == NULL) return;
    if (in_waiting_state(s)) {
        if (--s->ticks_left <= 0) {
        	PRINT("Retrying\n");
            resend(s);
            set_ticks(s, s->ticks * 2); /* Exponential (double) retransmission */
        }
    } else { /* Connection idling */
   		if (--s->ticks_till_ping <= 0) {
   			ping(s); /* PING A LING LONG */
   			set_ticks(s, TICKS);
        } else {
        	PRINT("CLOSING CHANNEL DUE TO TIMEOUT\n");
            close_graceful(s);
            remove_channel(s->chan_num);
        }

    }
}


/* Run once every 20ms */
void cleaner(int trash){
	PRINT("Cleaning\n");
    for (int i = 1; i < CHANNEL_NUM; i++) {
            check_timer(get_channel_state(i));
    }
    if (home_channel_state.state != STATE_IDLE) {
            check_timer(&home_channel_state);
    }
}


void network_handler() {
	DataPayload dp;

	/* Gets data from the connection */
	uint8_t src = recv_pkt(&dp);
	if (!src) return; /* The cake was a lie */
    uint8_t cmd = dp.hdr.cmd;
	PRINT("KNoT>> Received packet from Thing: ");PRINTLN(src);
	PRINT("Data is ");PRINT(dp.dhdr.tlen);PRINT(" bytes long\n");
	PRINT("Received a ");PRINT(cmdnames[cmd]);PRINT(" command.\n");
	PRINT("Message for channel ");PRINTLN(dp.hdr.dst_chan_num);
	
	ChannelState *state = NULL;

	if (cmd == QACK) {
		state = &home_channel_state;/* Special case for Homechannel which only */
		state->remote_addr = src;   /* responds to QACKs */
		qack_handler(state, &dp);
		return;
	} 
	/* Grab state for requested channel */
	state = get_channel_state(dp.hdr.dst_chan_num);

	/* Check state, if non-existent ask to kindly close the connection */
	if (state == NULL){ 
		PRINT("Channel ");PRINT(dp.hdr.dst_chan_num);PRINT(" doesn't exist\n");
		state = &home_channel_state;
		state->remote_chan_num = dp.hdr.src_chan_num;
		state->remote_addr = src;
		close_graceful(state);
		return;
	}
  /* Verify packet is new */
	if (check_seqno(state, &dp) == 0) {
		PRINT("Old paclet\n");
		return;
	}

	/* Special case for disconnect */
	if (cmd == DISCONNECT) {
		if (state) remove_channel(state->chan_num);
		state = &home_channel_state;
		state->remote_addr = src; /* Rest of disconnect handled later */ 
	}

	switch(cmd) {
		case(QUERY):    	break;
		case(CONNECT): 	 	break;
		case(QACK):     	break;
		case(CACK):     	cack_handler(state, &dp); break;
		case(RESPONSE): 	response_handler(state, &dp); break;
		case(RSYN):		 	  response_handler(state, &dp); send_rack(state); break;
		// case(CMDACK):   	command_ack_handler(state,dp);break;
		case(PING):     	ping_handler(state, &dp); break;
		case(PACK):     	pack_handler(state, &dp); break;
		case(DISCONNECT): close_handler(state,&dp);	break;
		default: 			    PRINT("Unknown CMD type\n");
	}

}

void serial_service_search(DataPayload *dp){
	service_search(&home_channel_state, ((SerialQuery*)dp)->type); 
}

void serial_init_connection_to(DataPayload *dp){
	SerialConnect *sc = (SerialConnect*)dp->data;
	PRINT("Serial init conn to: ");PRINTLN(dp->data[1]);
	init_connection_to(sc->addr, sc->rate);
}

void serial_handler(){
	DataPayload *dp = (DataPayload *)serialpkt;
	PRINT("SERIAL> Serial command received.\n");
	unsigned short cmd = dp->hdr.cmd;
	PRINT("SERIAL> Packet length:");PRINTLN(dp->dhdr.tlen);
	PRINT("SERIAL> Message for channel ");PRINTLN(dp->hdr.dst_chan_num);

	switch (cmd) {
		case(SERIAL_SEARCH):  serial_service_search(dp);break;
		case(SERIAL_CONNECT): serial_init_connection_to(dp);break;
	}
}


void setup(){
	Serial.begin(38400);
	PRINT(">> Controller initialising...");
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
	attach_serial(serial_handler, serialpkt);
	set_timer(0.02, 0, &cleaner); /* Set cleaner to run every 2secs - TESTING */
	PRINT(">> Controller initialised!");
	}

void loop(){
	if (NETWORK_EVENT)
		network_handler();
	else if (SERIAL_EVENT)
		recv_serial();
}
