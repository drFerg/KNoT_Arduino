#include "knot_network_pan.h"
#include "cc1101.h"
#include <stdlib.h>
// #include <avr/sleep.h>
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define enable_RF_IRQ() 	attachInterrupt(0, radioISR, FALLING);
#define disable_RF_IRQ() 	detachInterrupt(0);

// #define enable_SRF_IRQ() 	attachInterrupt(0, sleepyRadioISR, FALLING);
// #define disable_SRF_IRQ() 	detachInterrupt(0);


#define LEDOUTPUT 4
#define MAX_TX_RETRIES 5
CC1101 radio;

int available = 0;  

void blink(){
  digitalWrite(LEDOUTPUT, HIGH);
  delay(250);
  digitalWrite(LEDOUTPUT, LOW);
}

void radioISR(void){
	disable_RF_IRQ();
	if (radio.rfState == RFSTATE_RX){
		available = 1;
	}
	enable_RF_IRQ();
}

int packetAvailable(){
	return available;
}

void set_dev_addr(int addr){
	radio.setDevAddress(addr, false);
	Serial.print(F("Device address now: "));Serial.println(radio.readDevAddress());
}

int init_link_layer(){
	// initialize the RF Chip
	radio.init();
	radio.setDevAddress(DEVICE_ADDRESS, false);
	// enable the address checks in the chip hardware
	radio.enableAddressCheck();
	// we only want to receive data in this script
	radio.setRxState();
	radio.rfState = RFSTATE_RX;
	// Enable wireless reception interrupt
	Serial.print(F("CC1101 Radio initialised\n"));
	Serial.print(F("  - Device Address: "));Serial.println(radio.devAddress);
	enable_RF_IRQ();
	return 1;
}

void copy_address(ChannelState *state){
   // state->remote_port = UDP_HDR->srcport;
   // uip_ipaddr_copy(&(state->remote_addr) , &(UDP_HDR->srcipaddr));
}

void copy_address_ab(Address a, Address b){
   
   // uip_ipaddr_copy((uip_ipaddr_t *)a , (uip_ipaddr_t *)b);
}

void copy_address_broad(Address a){
   // uip_ipaddr_copy((uip_ipaddr_t *)a, &broad);
}


/**Send a message to the connection in state **/
void send_on_channel(ChannelState *state, DataPayload *dp){
	disable_RF_IRQ();
	CCPACKET pkt;
	pkt.length = 2 + sizeof(PayloadHeader) + sizeof(DataHeader) + dp->dhdr.tlen;
	pkt.data[0] = state->remote_addr; //set dest addr
	pkt.data[1] = radio.devAddress;
	memcpy(&(pkt.data[2]), dp, (pkt.length - 2)); // append payload
	bool txd = false;
	int tries = MAX_TX_RETRIES;

	while((!txd) && tries > 0){
		txd = radio.sendData(pkt);
		tries--;
	}
	if (txd){
		Serial.print(F("RADIO>> Sent packet to: "));Serial.println(pkt.data[0]);
		Serial.print(F("RADIO>> Tries: "));Serial.println(5 - tries);
		Serial.print(F("RADIO>> Sent "));Serial.print(pkt.length);Serial.print(" bytes\n");
		blink();
	}
	else Serial.print(F("RADIO>> Transmission failed???\n"));
	enable_RF_IRQ();
}

void broadcast(ChannelState *state, DataPayload *dp){
	disable_RF_IRQ();
	CCPACKET pkt;
	pkt.length = 2 + sizeof(PayloadHeader) + sizeof(DataHeader) + dp->dhdr.tlen;
	pkt.data[0] = 0; // set broadcast addr
	pkt.data[1] = radio.devAddress;
	memcpy(&(pkt.data[2]), dp, (pkt.length - 2)); //append payload
	bool txd = false;
	int tries = MAX_TX_RETRIES;

	while((!txd) && tries > 0){
		txd = radio.sendData(pkt);
		tries--;
	}
	if (txd){
		Serial.print(F("RADIO>> Sent broadcast packet to network\n"));
		Serial.print(F("RADIO>> Sent "));Serial.print(pkt.length);Serial.print(" bytes\n");
		blink();
	}
	else Serial.print(F("RADIO>> Transmission failed???\n"));
	enable_RF_IRQ();
}

void resend(ChannelState *state){
	send_on_channel(state,&(state->packet));
}

void send_uni(ChannelState *state, DataPayload *dp){
   // int dplen = sizeof(PayloadHeader) + sizeof(DataHeader) + uip_ntohs(dp->dhdr.tlen);
   // uip_udp_packet_send(udp_conn, (char*)dp, dplen);
   // PRINTF("Sent %s to ipaddr=%d.%d.%d.%d:%u\n", 
   //    cmdnames[dp->hdr.cmd], 
   //    uip_ipaddr_to_quad(&(udp_conn->ripaddr)),
   //    uip_htons(udp_conn->rport));
}

int recv_pkt(DataPayload *dp){
	CCPACKET packet;
	if (!packetAvailable()) return 0;
	disable_RF_IRQ();
	blink();
	available = 0;
	int len = radio.receiveData(&packet);
	if(len > 0) {
		//Serial.print("RADIO>> Data recvd: ");Serial.println(len);
		if (packet.crc_ok && (packet.length > 0)){
		    memcpy(dp, &(packet.data[2]), (packet.length - 2));
		    Serial.print(F("RADIO>> Pkt for addr: "));Serial.println(packet.data[0]);
		  	enable_RF_IRQ();
		  	return packet.data[1];
  		} else {
  			Serial.print(F("RADIO>> Bad packet.\n"));
  			enable_RF_IRQ();
  			return 0;
  		}
  	}
  	else {
  		enable_RF_IRQ();
  		return 0;
  	}
}

// int sleep_recv_pkt(DataPayload *dp){
// 	CCPACKET packet;
// 	if (packetAvailable()) recv_pkt(dp);
// 	Serial.print("Going to sleep....");
// 	delay(100);
// 	sleep_enable();
// 	enable_SRF_IRQ();
// 	 0, 1, or many lines of code here 
// 	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
// 	cli();
// 	sleep_bod_disable();
// 	sei();
// 	sleep_cpu();
// 	/* wake up here */
// 	sleep_disable();
// 	Serial.print("AWAKE!!!\n");
// 	recv_pkt(dp);	
// }