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
char buffer[BUFF_LEN];
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
			memset(buffer, '\0', BUFF_LEN);
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
			buffer[index++] = data;
		}
	} 
	else if (status == ESC_NEXT){
		buffer[index++] = data;
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
				memcpy(callbackPkt, buffer, index);
				packetCallback();
			}
			status = WAITING_ON_START;
		}
	}
}