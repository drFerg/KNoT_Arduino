#include <stdlib.h>
#include <string.h>
#include <TimerOne.h>
#include "EEPROM.h"
#include "cc1101.h"
#include <MemoryFree.h>

#include "cpu_temp.h"
#include "knot_protocol.h"

#include "knot_network_pan.h"
#include "knot_network.h"
#include "channeltable.h"
#include "callback_timer.h"
#include "LED.h"


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

#define LIGHT A0
#define TEMP_PIN A7
#define DEVICE_ADDRESS 3
char sensor_name[] = "Bedroom";
uint8_t sensor_type = TEMP;

ChannelState home_channel_state;

float ambientTemp(){
	int reading = analogRead(TEMP_PIN);
	return (100 * reading * 3.3)/1024;

}

void query_handler(ChannelState *state, DataPayload *dp){
	QueryMsg *q = (QueryMsg* )(dp->data);
	Serial.println(q->type);
	if (q->type != sensor_type){
		Serial.print(F("Query doesn't match type\n"));
		return;
	} else {
		Serial.print(F("Query matches type\n"));
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
	Serial.print(cm->name);Serial.print(F(" wants to connect from channel "));Serial.println(dp->hdr.src_chan_num);
	Serial.print(F("Replying on channel "));Serial.println(state->chan_num);
	/* Request src must be saved to message back */

	state->remote_chan_num = dp->hdr.src_chan_num;
	if (cm->rate > DATA_RATE){
		state->rate = cm->rate;
	} else{
		state->rate = DATA_RATE;
		Serial.println(cm->rate);
	}
	DataPayload *new_dp = &(state->packet);
	ConnectACKMsg *ck = (ConnectACKMsg *)&(new_dp->data);
	clean_packet(new_dp);

	strcpy(ck->name,sensor_name); // copy name
	ck->accept = 1;
	dp_complete(new_dp, state->chan_num, state->remote_chan_num, 
				CACK, sizeof(ConnectACKMsg));
	send_on_knot_channel(state,new_dp);
	state->state = STATE_CONNECT;
	// Set up timer to ensure reliability
	state->timer = set_timer(TIMEOUT, state->chan_num, &reliable_retry);
	if (state->timer == -1){
		Serial.print(F("ERROR>> Setting timer failed!!\n"));
	}// Shouldn't happen...
}

void cack_handler(ChannelState *state, DataPayload *dp){
	if (state->state != STATE_CONNECT){
		Serial.print(F("Not in Connecting state\n"));
		return;
	}
	//Disable timer now that message has been received successfully
	remove_timer(state->timer);
	state->ticks = state->rate * PING_RATE;
	Serial.print(F("TX rate: "));Serial.println(state->rate);
	// Setup sensor polling
	state->timer = set_timer(state->rate, state->chan_num, &send_handler);
	if (state->timer == -1){
		Serial.print(F("ERROR>> Setting sensor timer failed!!\n"));
	}// Shouldn't happen...

	Serial.print(F(">>CONNECTION FULLY ESTABLISHED<<\n"));
	state->state = STATE_CONNECTED;
}

void send_value(ChannelState *state){
    DataPayload *new_dp = &(state->packet);
	ResponseMsg *rmsg = (ResponseMsg*)&(new_dp->data);
	clean_packet(new_dp);
	//state->ccb.callback(NULL, &data);
	rmsg->data = (int)getCPUTemp();
	//rmsg.data = analogRead(LIGHT);
	//rmsg.data = (int)ambientTemp();
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
    Serial.print(F("Sending data\n"));
    send_on_knot_channel(state, new_dp);
}

void rack_handler(ChannelState *state, DataPayload *dp){
	Serial.print(F("The other end is still alive!\n"));
}

void network_handler(){
	DataPayload dp;
	uint8_t src = recv_pkt(&dp);
	if (src){
		Serial.print(F("KNoT>> Received packet from "));Serial.println(src);
	}
	else {
		return;
	}
	
	Serial.print(F("Data is "));Serial.print(dp.dhdr.tlen);Serial.print(F(" bytes long\n"));
	unsigned short cmd = dp.hdr.cmd;        // only a byte so no reordering :)
	Serial.print(F("Received a "));Serial.print(cmdnames[cmd]);Serial.print(F(" command.\n"));
	Serial.print(F("Message for channel "));Serial.println(dp.hdr.dst_chan_num);
	
	ChannelState *state = NULL;
	if (cmd == DISCONNECT){
		state = get_channel_state(dp.hdr.dst_chan_num);
		if (state){
			remove_channel(state->chan_num);
		}
		state = &home_channel_state;
		state->remote_addr = src;
  	} /* Special case for Homechannel which only responds to QACKs */
	else if (dp.hdr.dst_chan_num == HOME_CHANNEL){
		if (cmd == QUERY){
			state = &home_channel_state;
			state->remote_addr = src;
  		}
  		else if (cmd == CONNECT){
  			state = new_channel();
  			Serial.print(F("Sensor: New Channel\n"));
  			state->remote_addr = src;
  		}
  	}
		else {
		state = get_channel_state(dp.hdr.dst_chan_num);
		if (state == NULL){
			Serial.print(F("Channel doesn't exist\n"));
			return;
		}
		// 	//copy_link_address(state); // CHECK IF RIGHT CONNECTION
		
 	}

	Serial.print(F("Seqno: "));Serial.println(dp.hdr.seqno);
	if (check_seqno(state, &dp) == 0){
		Serial.print(F("Oh no\n"));
		return;
	}
	
	/* PUT IN QUERY CHECK FOR TYPE */
	Serial.print(F("Memory left:"));
	Serial.println(freeMemory());
	switch(cmd){
		case(QUERY):   		query_handler(state, &dp);		break;
		case(CONNECT): 		connect_handler(state, &dp);	break;
		case(CACK):   		cack_handler(state, &dp);		break;
		case(PING):   		ping_handler(state, &dp);		break;
		case(PACK):   		pack_handler(state, &dp);		break;
		case(RACK):			rack_handler(state, &dp);		break;
		case(DISCONNECT): 	close_handler(state,&dp);		break;
		default:											break;
	}

}

void send_handler(int chan){
	ChannelState *s = get_channel_state(chan);
	send_value(s);
	set_timer(s->rate, s->chan_num, &send_handler);
	Serial.print(F("SENT\n"));
}

void reliable_retry(int chan){
	ChannelState *s = get_channel_state(chan);
	if (s == NULL)return;
	if (s->state % 2 != 0){ // Waiting for response state...
		Serial.print(F("Retrying...\n"));
		resend(s); // Assume failed, retry and set timer again
		set_timer(TIMEOUT, s->chan_num, &reliable_retry);
	}
}

void setup(){
	Serial.begin(38400);
	Serial.println(F(">> Sensor initialising..."));
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
	Serial.println(F(">> Initialised"));
}

void loop(){
	if (NETWORK_EVENT)
		network_handler();
	else if (TIMER_EVENT)
		timer_handler();

}