#include <stdlib.h>
#include <string.h>
#include <serialPrintf.h>

#include "EEPROM.h"
#include "cc1101.h"



#include "knot_protocol.h"
#include <TimerOne.h>
#include "knot_network_pan.h"
#include "knot_network.h"
#include "channeltable.h"

#define DEBUG 1

#if DEBUG

#define PRINTF(...) serialPrintf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define PING_WAIT 3
#define TIMER_INTERVAL 3
#define HOMECHANNEL 0

#define LEDOUTPUT 4
#define LEDONOFF 3
#define RED A5
#define GREEN A3
#define BLUE A4
#define DEVICE_ADDRESS 10

#define NETWORK_EVENT packetAvailable()
#define SERIAL_EVENT Serial.available()
#define TIMER_EVENT 0

char controller_name[] = "The Boss";
ChannelState home_channel_state;
int connected = 0;
int serial_ready = 0;
char buf[50];
int serial_index = 0;
int addr = 0;

void blinker(){
      digitalWrite(LEDOUTPUT, HIGH);
      delay(100);
      digitalWrite(LEDOUTPUT, LOW);
      delay(100);
}

void qack_handler(ChannelState *state, DataPayload *dp){
	if (state->state != STATE_QUERY) {
		Serial.print("Not in Query state\n");
		return;
	}
	Serial.print("Query ACK received\n");
	state->ticks = 100;
	QueryResponseMsg *qr = (QueryResponseMsg *)&dp->data;

	Serial.print("Sensor name: ");Serial.println(qr->name);
	Serial.print("Sensor type: ");Serial.println(qr->type);
	char str[50];
	sprintf(str,"[QACK,%d,%d,%s,%d]\n", state->remote_addr,
									state->remote_chan_num,
									qr->name,
									qr->type);
	Serial.print(str);
	addr = state->remote_addr;
	// char *data = (char *)dp;
	// for (int i = 0; i < sizeof(QueryResponseMsg); i++){
	// 	Serial.write(data[i]);
	// }
	// Serial.println();
	//state->state = STATE_IDLE;
	// process_post(state->ccb.client_process, KNOT_EVENT_SERVICE_FOUND, &sc);
	//create_channel(state, dp);
}

void cack_handler(ChannelState *state, DataPayload *dp){
	if (state->state != STATE_CONNECT){
		Serial.print("Not in Connecting state\n");
		return;
	}
	ConnectACKMsg *ck = (ConnectACKMsg*)(dp->data);
	if (ck->accept == 0){
		Serial.print("SCREAM! THEY DIDN'T EXCEPT!!");
	}
	Serial.print(ck->name);Serial.print(" accepts connection request on channel "); Serial.println(dp->hdr.src_chan_num);
	state->remote_chan_num = dp->hdr.src_chan_num;

	DataPayload *new_dp = &(state->packet);
	clean_packet(new_dp);
	new_dp->hdr.src_chan_num = state->chan_num;
	new_dp->hdr.dst_chan_num = state->remote_chan_num;
	//dp_complete(new_dp,10,QACK,1);
    new_dp->hdr.cmd = CACK; 
    new_dp->dhdr.tlen = 0;
	send_on_knot_channel(state,new_dp);
	state->state = STATE_CONNECTED;
	state->ticks = 100;
	connected = 1;
	//Set up ping timeouts for liveness
}

void response_handler(ChannelState *state, DataPayload *dp){
	if (state->state != STATE_CONNECTED && state->state != STATE_PING){
		PRINTF("Not connected to device!\n");
		return;
	}
	state->ticks = 100;
	ResponseMsg *rmsg = (ResponseMsg *)dp->data;
	Serial.print(rmsg->name); Serial.print(": "); Serial.println(rmsg->data);
	/*RESET PING TIMER*/
}

void service_search(ChannelState* state, uint8_t type){

  DataPayload *new_dp = &(state->packet); 
  clean_packet(new_dp);
  //dp_complete(new_dp,10,QACK,1); new_dp->hdr.src_chan_num = state->chan_num;
  new_dp->hdr.dst_chan_num = 0; 
  (new_dp)->hdr.cmd = QUERY;
  (new_dp)->dhdr.tlen = sizeof(QueryMsg); 
  QueryMsg *q = (QueryMsg *) new_dp->data;

  q->type = type;
  strcpy(q->name, controller_name);
  knot_broadcast(state,new_dp);
  state->state = STATE_QUERY;
  state->ticks = 100;
}

void init_connection_to(ChannelState* state, uint8_t addr, int rate){
	ChannelState * s = new_channel();
	if (state == NULL) return;

	s->remote_addr = addr;
	s->rate = rate;
	DataPayload *new_dp = &(s->packet);
	clean_packet(new_dp);

	ConnectMsg *cm = (ConnectMsg *)(new_dp->data);
	strcpy(cm->name, controller_name);
	cm->rate = rate;
	new_dp->hdr.dst_chan_num = 0;
	new_dp->hdr.src_chan_num = s->chan_num;
 	new_dp->hdr.cmd = CONNECT; 
    new_dp->dhdr.tlen = sizeof(ConnectMsg);
    Serial.print("Sending connect request\n");
    send_on_knot_channel(s,new_dp);
    s->state = STATE_CONNECT;
	s->ticks = 10;
}
void network_handler(){
	DataPayload dp;

	/* Gets data from the connection */
	uint8_t src = recv_pkt(&dp);
	if (src){
		Serial.print("KNoT>> Received packet from ");Serial.println(src);
	}
	else {
		return;
	}

	Serial.print("Data is ");Serial.print(dp.dhdr.tlen);Serial.print(" bytes long\n");
	unsigned short cmd = dp.hdr.cmd;
	Serial.print("Received a ");Serial.print(cmdnames[cmd]);Serial.print(" command.\n");
	Serial.print("Message for channel ");Serial.println(dp.hdr.dst_chan_num);
	
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
  	else if (dp.hdr.dst_chan_num == HOMECHANNEL && cmd == QACK){
		state = &home_channel_state;
		state->remote_addr = src;
  	} /* The rest of the channels */
	else{
		state = get_channel_state(dp.hdr.dst_chan_num);
		if (state == NULL){
			Serial.print("Channel ");Serial.print(dp.hdr.dst_chan_num);Serial.print(" doesn't exist\n");
			return;
		}
		if (check_seqno(state, &dp) == 0) {
			Serial.print("OH NOES\n");
			return;
		} else { //CHECK IF RIGHT CONNECTION
			//copy_link_address(state);
		}
	}
	//continue;
	switch(cmd){
		case(QUERY):    	break;
		case(CONNECT): 	 	break;
		case(QACK):     	qack_handler(state, &dp);break;
		case(CACK):     	cack_handler(state, &dp);break;
		case(RESPONSE): 	response_handler(state, &dp);break;
		// case(CMDACK):   	command_ack_handler(state,dp);break;
		case(PING):     	ping_handler(state, &dp);break;
		// case(PACK):     	pack_handler(state, &dp);break;
		// case(DISCONNECT): 	close_handler(state,&dp);
	}

}


void read_serial(){
	while (Serial.available()){
		buf[serial_index++] = Serial.read();
		if (buf[serial_index-1] == ';'){
			serial_ready = 1;
		}
	}
	

}


void serial_handler(){
	Serial.println(buf);
	if (buf[0] == 's'){
		service_search(&home_channel_state, TEMP);
	}
	else if (buf[0] == 'c'){
		init_connection_to(&home_channel_state, addr, 10);
	}
	// switch (inByte){
	// 	case 's': service_search(&home_channel_state, );
	// 	default: Serial.print("Invalid command\n");Serial.print(inByte);
	// }
}


/** PUBLIC METHODS **/
void connect_to_dev(uint8_t addr, int rate, void(*callback)(byte*data)){
	init_connection_to(&home_channel_state, addr, rate);
}


void setColor(int red, int green, int blue)
{
	analogWrite(RED, 255 - red);
	analogWrite(GREEN, 255 - green);
	analogWrite(BLUE, 255 - blue);
}

void setup(){

	Serial.begin(38400);
	randomSeed(analogRead(0));
	pinMode(LEDOUTPUT, OUTPUT);
	digitalWrite(LEDOUTPUT, LOW);

	pinMode(LEDONOFF, OUTPUT);
	digitalWrite(LEDONOFF, HIGH);

	Serial.println("setup done");
	init_table();
	init_knot_network();
	set_dev_addr(random(1,256));
	blinker();
	home_channel_state.chan_num = 0;
	home_channel_state.remote_chan_num = 0;
	home_channel_state.state = STATE_CONNECTED;
	home_channel_state.remote_addr = 5;
	home_channel_state.rate = 60;

	}

unsigned long thresh = 3000;
unsigned long timer = 0;
void loop(){
	if (NETWORK_EVENT)
		network_handler();
	else if (SERIAL_EVENT)
		read_serial();
	else if (serial_ready){
		serial_ready = 0;
		serial_index = 0;
		memset(&buf,50,'\0');
		serial_handler();
	}
	// else if (TIMER_EVENT)
	// 	check_timer();
}
