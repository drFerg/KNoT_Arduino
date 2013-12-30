#include <stdlib.h>
#include <string.h>
#include <TimerOne.h>
#include "EEPROM.h"
#include "cc1101.h"

#include "cpu_temp.h"
#include "knot_protocol.h"

#include "knot_network_pan.h"
#include "knot_network.h"
#include "channeltable.h"
#include "callback_timer.h"
#include "LED.h"

#if DEBUG
#define PRINT(...) Serial.print(__VA_ARGS__)
#define PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define PRINT(...)
#define PRINTLN(...)
#endif

#define TIMEOUT 0.5
#define PING_WAIT 3
#define TIMER_INTERVAL 3
#define HOME_CHANNEL 0
#define BUFF 256

#define TIMER_INTERVAL 3
#define DATA_RATE  1
#define PING_RATE  1   // How many data intervals to wait before PING
#define RATE_CHANGE 1

#define NETWORK_EVENT packetAvailable()
#define TIMER_EVENT timer_expired()

#define RICE_COOK_PIN A0
#define TEMP_PIN A7

char sensor_name[] = "Bedroom";
uint8_t sensor_type = TEMP;

ChannelState home_channel_state;

float ambientTemp(){
	int reading = analogRead(TEMP_PIN);
	return (100 * reading * 3.3)/1024;
}

int isCooking(){
	return (analogRead(RICE_COOK_PIN) > 100 ? 1 : 0);
}

void query_handler(ChannelState *state, DataPayload *dp){
	QueryMsg *q = (QueryMsg*)(dp->data);
	if (q->type != sensor_type){
		PRINT(F("Query doesn't match type\n"));
		return;
	} else {
		PRINT(F("Query matches type\n"));
	}
	DataPayload *new_dp = &(state->packet);
	QueryResponseMsg *qr = (QueryResponseMsg*)&(new_dp->data);
	clean_packet(new_dp);

	strcpy(qr->name, sensor_name); // copy name
	qr->type = sensor_type;
	qr->rate = DATA_RATE;
	dp_complete(new_dp, state->chan_num, dp->hdr.src_chan_num, 
				QACK, sizeof(QueryResponseMsg));
	send_on_knot_channel(state, new_dp);
}


void connect_handler(ChannelState *state, DataPayload *dp){
	ConnectMsg *cm = (ConnectMsg*)dp->data;
	PRINT(cm->name);PRINT(F(" wants to connect from channel "));
	PRINTLN(dp->hdr.src_chan_num);
	PRINT(F("Replying on channel "));PRINTLN(state->chan_num);
	/* Request src must be saved to message back */

	state->remote_chan_num = dp->hdr.src_chan_num;
	if (cm->rate > DATA_RATE){
		state->rate = cm->rate;
		PRINT("The rate is set to: ");
		PRINTLN(state->rate);
	} else {
		state->rate = DATA_RATE;
		PRINTLN(cm->rate);
	}
	DataPayload *new_dp = &(state->packet);
	ConnectACKMsg *ck = (ConnectACKMsg *)&(new_dp->data);
	clean_packet(new_dp);
	dp_complete(new_dp, state->chan_num, state->remote_chan_num, 
				CACK, sizeof(ConnectACKMsg));

	strcpy(ck->name,sensor_name); // copy name
	ck->accept = 1;
	send_on_knot_channel(state,new_dp);
	state->state = STATE_CONNECT;
	// Set up timer to ensure reliability
	state->timer = set_timer(TIMEOUT, state->chan_num, &reliable_retry);
	if (state->timer == -1){
		PRINT(F("ERROR>> Setting timer failed!!\n"));
	}// Shouldn't happen...
}

void cack_handler(ChannelState *state, DataPayload *dp){
	if (state->state != STATE_CONNECT){
		PRINT(F("Not in Connecting state\n"));
		return;
	}
	//Disable timer now that message has been received successfully
	remove_timer(state->timer);
	state->ticks = state->rate * PING_RATE;
	PRINT(F("TX rate: "));PRINTLN(state->rate);
	// Setup sensor polling
	state->timer = set_timer(state->rate, state->chan_num, &send_handler);
	if (state->timer == -1){
		PRINT(F("ERROR>> Setting sensor timer failed!!\n"));
	}// Shouldn't happen...

	PRINT(F(">>CONNECTION FULLY ESTABLISHED<<\n"));
	state->state = STATE_CONNECTED;
}

void send_value(ChannelState *state){
    DataPayload *new_dp = &(state->packet);
	ResponseMsg *rmsg = (ResponseMsg*)&(new_dp->data);
	clean_packet(new_dp);
	//state->ccb.callback(NULL, &data);
	//rmsg->data = (int)isCooking();
	//rmsg->data = analogRead(LIGHT);
	rmsg->data = (int)getCPUTemp();
	strcpy(rmsg->name, sensor_name);
	
	// Send a Response SYN or Response
	if(state->ticks == 0){
	    new_dp->hdr.cmd = RSYN; // Send to ensure controller is still out there
	    state->ticks = state->rate * PING_RATE;
	    //TODO:Set additional timer to init full PING check
    } else{
    	new_dp->hdr.cmd = RESPONSE;
    	state->ticks--;
    }
    new_dp->hdr.src_chan_num = state->chan_num;
		new_dp->hdr.dst_chan_num = state->remote_chan_num;
    new_dp->dhdr.tlen = sizeof(ResponseMsg);
    PRINT(F("Sending data\n"));
    send_on_knot_channel(state, new_dp);
}

void rack_handler(ChannelState *state, DataPayload *dp){
	PRINT(F("The other end is still alive!\n"));
}

void network_handler(){
	DataPayload dp;
	uint8_t src = recv_pkt(&dp);
	if (src){
		PRINT(F("KNoT>> Received packet from "));
		PRINTLN(src);
	}
	else {
		return;
	}
	
	PRINT(F("Data is "));PRINT(dp.dhdr.tlen);PRINT(F(" bytes long\n"));
	unsigned short cmd = dp.hdr.cmd;        // only a byte so no reordering :)
	PRINT(F("Received a "));
	PRINT(cmdnames[cmd]);
	PRINT(F(" command.\n"));
	PRINT(F("Message for channel "));
	PRINTLN(dp.hdr.dst_chan_num);
	
	ChannelState *state = NULL;
	/* Always allow disconnections to prevent crazies */
	if (cmd == DISCONNECT){
		state = get_channel_state(dp.hdr.dst_chan_num);
		if (state){
			remove_channel(state->chan_num);
		}
		state = &home_channel_state;
		state->remote_addr = src;
  	} else if (dp.hdr.dst_chan_num == HOME_CHANNEL){
  		/* Homechannel only responds to QUERYs and CONNECTs */
		if (cmd == QUERY){
			state = &home_channel_state;
			state->remote_addr = src;
  		} else if (cmd == CONNECT){
  			state = new_channel();
  			PRINT(F("Sensor: New Channel\n"));
  			state->remote_addr = src;
  		} else return; //Otherwise quit
  	} else {
		state = get_channel_state(dp.hdr.dst_chan_num);
		if (state == NULL){
			PRINT(F("Channel doesn't exist\n"));
			return;
		} else if (check_seqno(state, &dp) == 0){
			PRINT(F("Oh no\n"));
			return;
		} else {
		 // CHECK IF RIGHT CONNECTION
		}
 	}

	
	/* PUT IN QUERY CHECK FOR TYPE */
	switch(cmd){
		case(QUERY):   		query_handler(state, &dp);		break;
		case(CONNECT): 		connect_handler(state, &dp);	break;
		case(CACK):   		cack_handler(state, &dp);		break;
		case(PING):   		ping_handler(state, &dp);		break;
		case(PACK):   		pack_handler(state, &dp);		break;
		case(RACK):			rack_handler(state, &dp);		break;
		case(DISCONNECT): 	close_handler(state, &dp);		break;
		default:			PRINT(F("Unknown CMD type\n"));
	}

}

void send_handler(int chan){
	ChannelState *s = get_channel_state(chan);
	send_value(s);
	set_timer(s->rate, s->chan_num, &send_handler);
	PRINT(F("SENT\n"));
}

void reliable_retry(int chan){
	ChannelState *s = get_channel_state(chan);
	if (s == NULL)return;
	if (s->state % 2 != 0){ // Waiting for response state...
		PRINT(F("Retrying...\n"));
		resend(s); // Assume failed, retry and set timer again
		set_timer(TIMEOUT, s->chan_num, &reliable_retry);
	}
}

void setup(){
	Serial.begin(38400);
	PRINTLN(F(">> Sensor initialising..."));
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
	init_timer();
	PRINTLN(F(">> Initialised"));
}

void loop(){
	if (NETWORK_EVENT)
		network_handler();
	else if (TIMER_EVENT)
		run_all_expired_timers();

}