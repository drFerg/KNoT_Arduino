#include "serialPacket.h"
#include <Arduino.h>

#define START_FLAG 0x12
#define END_FLAG 0x13
#define ESC_FLAG 0x7D

#define WAITING_ON_START 0
#define IN_MSG 1
#define MSG_COMPLETE 2
#define ESC_NEXT 3

#define BUFF_LEN 50
char recv_buffer[BUFF_LEN];
byte send_buffer[BUFF_LEN];

char *callbackPkt;
uint8_t index = 0;
uint8_t status = 0;


void (*packetCallback)(void);

void attach_serial(void(*callback)(void), char *pkt){
	packetCallback = callback;
	callbackPkt = pkt;
}



uint8_t process_byte(char data){
	if (status == WAITING_ON_START){
		if (data == START_FLAG){
			status = IN_MSG;
			index = 0;
			memset(recv_buffer, '\0', BUFF_LEN);
		} 
	}
	else if (status == IN_MSG){
		if (data == END_FLAG){
			status = MSG_COMPLETE;
			index--;
		} 
		else if (data == ESC_FLAG){
			status = ESC_NEXT;
		} 
		else {
			recv_buffer[index++] = data;
		}
	} 
	else if (status == ESC_NEXT){
		recv_buffer[index++] = data;
	}
	return status;
}

int crc_ok(){
	return 1;
}

void recv_serial(){
	char data;
	while (Serial.available() > 0){
			data = Serial.read();
			process_byte(data);
		if (status == MSG_COMPLETE){
			if(crc_ok()){
				memcpy(callbackPkt, recv_buffer, index);
				packetCallback();
			}
			status = WAITING_ON_START;
		}
	}
}

void write_to_serial(char *data, int len){
	int d = 0;
	int b = 0;
	memset(send_buffer, '\0', BUFF_LEN);
	send_buffer[b++] = START_FLAG;
	for (;d < len; d++){
		if (data[d] == START_FLAG || data[d] == END_FLAG || data[d] == ESC_FLAG){
			send_buffer[b++] = ESC_FLAG;
		}
		send_buffer[b++] = data[d];
	}
	send_buffer[b++] = END_FLAG;
	Serial.write(send_buffer, b);
}