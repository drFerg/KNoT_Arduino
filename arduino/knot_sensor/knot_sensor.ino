#include <stdlib.h>
#include <string.h>
#include <serialPrintf.h>
#include <TimerOne.h>
#include "EEPROM.h"
#include "cc1101.h"


#include "cpu_temp.h"
#include "knot_protocol.h"

#include "knot_network_pan.h"
#include "knot_network.h"
#include "channeltable.h"
#include "response_timer.h"
#define DEBUG 1

#if DEBUG

#define PRINTF(...) serialPrintf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define PING_WAIT 3
#define TIMER_INTERVAL 3
#define HOMECHANNEL 0
#define BUFF 256

#define TIMER_INTERVAL 3
#define DATA_RATE  1
#define PING_RATE  5   // How many data intervals to wait before PING
#define RATE_CHANGE 1

#define LEDOUTPUT 4
#define LEDONOFF 3
#define RED A5
#define GREEN A3
#define BLUE A4

#define LIGHT A0
#define TEMP_PIN A7
#define DEVICE_ADDRESS 3
char sensor_name[] = "Bedroom";
uint8_t sensor_type = TEMP;

ChannelState home_channel_state;
ChannelState *sending;
int sensing = 0;

float ambientTemp(){
	int reading = analogRead(TEMP_PIN);
	return (100 * reading * 3.3)/1024;

}
void blinker(){
      digitalWrite(LEDOUTPUT, HIGH);
      delay(100);
      digitalWrite(LEDOUTPUT, LOW);
      delay(100);
}
void query_handler(ChannelState *state, DataPayload *dp){
	QueryMsg *q = (QueryMsg* )(dp->data);
	Serial.println(q->type);
	if (q->type != sensor_type){
		Serial.print("Query doesn't match type\n");
		return;
	}
	else {Serial.print("Query matches type\n");}
	DataPayload *new_dp = &(state->packet);
	clean_packet(new_dp);
	QueryResponseMsg qr;

	qr.type = sensor_type;
	strcpy(qr.name, sensor_name); // copy name
	qr.rate = DATA_RATE;
	new_dp->hdr.dst_chan_num = dp->hdr.src_chan_num; 
    new_dp->hdr.cmd = QACK; 
    new_dp->dhdr.tlen = sizeof(QueryResponseMsg);
    memcpy(new_dp->data, &qr, sizeof(QueryResponseMsg));
    Serial.print("Responding to query\n");
	send_on_knot_channel(state, new_dp);
}


void connect_handler(ChannelState *state, DataPayload *dp){
	ConnectMsg *cm = (ConnectMsg*)dp->data;
	Serial.print(cm->name);Serial.print(" wants to connect from channel ");Serial.println(dp->hdr.src_chan_num);
	Serial.print("Replying on channel ");Serial.println(state->chan_num);
	/* Request src must be saved to message back */
	//state->ccb.callback = home_channel_state.ccb.callback;
	state->remote_chan_num = dp->hdr.src_chan_num;
	if (cm->rate > DATA_RATE){
		state->rate = cm->rate;
	}else{
		state->rate = DATA_RATE;
		Serial.println(cm->rate);
	}
	DataPayload *new_dp = &(state->packet);
	ConnectACKMsg ck;
	strcpy(ck.name,sensor_name); // copy name
	ck.accept = 1;
	new_dp->hdr.src_chan_num = state->chan_num;
	new_dp->hdr.dst_chan_num = state->remote_chan_num;

	//dp_complete(new_dp,10,QACK,1);
    (new_dp)->hdr.cmd = CACK; 
    
    (new_dp)->dhdr.tlen = sizeof(ConnectACKMsg);
    memcpy(&(new_dp->data),&ck,sizeof(ConnectACKMsg));
	send_on_knot_channel(state,new_dp);
	state->state = STATE_CONNECT;
}

void cack_handler(ChannelState *state, DataPayload *dp){
	if (state->state != STATE_CONNECT){
		PRINTF("Not in Connecting state\n");
		return;
	}
	state->ticks = state->rate * PING_RATE;
	Serial.print("TX rate: ");Serial.println(state->rate);
	// Setup sensor polling HERE
	int ret = set_timer(state->rate, state->chan_num);
	if (ret != -1) Serial.print("Set timer\n");
	Serial.print(">>CONNECTION FULLY ESTABLISHED<<\n");
	state->state = STATE_CONNECTED;
	sending = state;
	sensing = 1;
}

void send_handler(ChannelState* state){
    DataPayload *new_dp = &(state->packet);
	ResponseMsg rmsg;
	uint16_t data = 0;
	//state->ccb.callback(NULL, &data);
	//rmsg.data = (int)getCPUTemp();
	//rmsg.data = analogRead(LIGHT);
	rmsg.data = (int)ambientTemp();
	strcpy(rmsg.name,sensor_name);

	new_dp->hdr.src_chan_num = state->chan_num;
	new_dp->hdr.dst_chan_num = state->remote_chan_num;
    new_dp->hdr.cmd = RESPONSE; 
    new_dp->dhdr.tlen = sizeof(ResponseMsg);
    memcpy(&(new_dp->data),&rmsg,sizeof(ResponseMsg));
    Serial.print("Sending data\n");
    send_on_knot_channel(state, new_dp);
}

void network_handler(){
	unsigned short cmd;
	DataPayload dp;
	uint8_t src;
	unsigned long thresh = 500;
	unsigned long timer = 0;
	for(;;){
		if (timer_expired()){
			int chan = get_next_expired();
			while (chan){
				Serial.print("Channel timer expired: ");Serial.println(chan);
				ChannelState *s = get_channel_state(chan);
				send_handler(s);
				set_timer(s->rate, s->chan_num);
				chan = get_next_expired();
				Serial.print("SENT\n");
			}
		}
		ChannelState *state = NULL;
		src = recv_pkt(&dp);
		if (src){
			Serial.print("KNoT>> Received packet from ");Serial.println(src);
		}
		else {
			Serial.print(".");
			delay(1000);
			continue;
		}
		
		Serial.print("Data is ");Serial.print(dp.dhdr.tlen);Serial.print(" bytes long\n");
		cmd = dp.hdr.cmd;        // only a byte so no reordering :)
		Serial.print("Received a ");Serial.print(cmdnames[cmd]);Serial.print(" command.\n");
		Serial.print("Message for channel ");Serial.println(dp.hdr.dst_chan_num);
		
		if (cmd == DISCONNECT){
			state = get_channel_state(dp.hdr.dst_chan_num);
			if (state){
				remove_channel(state->chan_num);
			}
			state = &home_channel_state;
			state->remote_addr = src;
	  	} /* Special case for Homechannel which only responds to QACKs */
		else if (dp.hdr.dst_chan_num == HOMECHANNEL){
			if (cmd == QUERY){
				state = &home_channel_state;
				state->remote_addr = src;
	  		}
	  		else if (cmd == CONNECT){
	  			state = new_channel();
	  			Serial.print("Sensor: New Channel\n");
	  			state->remote_addr = src;
	  		}
	  	}
  		else {
			state = get_channel_state(dp.hdr.dst_chan_num);
			if (state == NULL){
				Serial.print("Channel doesn't exist\n");
				return;
			}
			// 	//copy_link_address(state); // CHECK IF RIGHT CONNECTION
			
	 	}

		serialPrintf("Seqno: %d\n", dp.hdr.seqno);
		if (cmd == QUERY)
			query_handler(state, &dp);
		else if (cmd == CONNECT)
			connect_handler(state, &dp);
		else if (cmd == CACK)
			cack_handler(state,&dp);
		// if (check_seqno(state, &dp) == 0){
		// 	Serial.print("Oh no\n");
		// }
		//continue;
		/* PUT IN QUERY CHECK FOR TYPE */
		// switch(cmd){
		//     //case(QUERY):   		query_handler(state,&dp, src);	break;
		//  //  	case(CONNECT): 		connect_handler(state,&dp);		break;
		// 	// case(CACK):   		cack_handler(state, &dp);		break;
		// 	case(PING):   		ping_handler(state, &dp);		break;
		// 	// case(PACK):   		pack_handler(state, &dp);		break;
		// 	// case(DISCONNECT): 	close_handler(state,&dp);		break;
		// }
	}
}

void rgbSetup(){
	pinMode(RED, OUTPUT);
	pinMode(GREEN, OUTPUT);
	pinMode(BLUE, OUTPUT);

	analogWrite(RED, 255);
	analogWrite(GREEN, 255);
	analogWrite(BLUE, 255);

	analogWrite(RED, 0);
	delay(100);
	analogWrite(GREEN, 0);
	delay(100);
	analogWrite(BLUE, 0);
	delay(100);

	setColour(0,0,0);
}

void setColour(int red, int green, int blue)
{
	analogWrite(RED, 255 - red);
	analogWrite(GREEN, 255 - green);
	analogWrite(BLUE, 255 - blue);
}

void setup(){
	Serial.begin(9800);
	Serial.println("start");
	randomSeed(analogRead(0));
	pinMode(LEDOUTPUT, OUTPUT);
	digitalWrite(LEDOUTPUT, LOW);
	pinMode(LEDONOFF, OUTPUT);
	digitalWrite(LEDONOFF, HIGH);
	//rgbSetup();

	Serial.println("setup done");
	init_table();
	init_knot_network();
	set_dev_addr(random(1,256));
	blinker();

	home_channel_state.chan_num = 0;
    home_channel_state.seqno = 0;
	init_timer();
}

void loop(){
	network_handler();
}