#include "callback_timer.h"
#include <Arduino.h>
#include <TimerOne.h>


#define NUM_OF_TIMERS 10
#define MAX_TIME_S 8 //Timer limit is 8.3....s
#define ONE_SECOND 1000000 //One second in microseconds
#define MAX_TIME_MS (MAX_TIME_S * ONE_SECOND)

int32_t timers[NUM_OF_TIMERS];
int events[NUM_OF_TIMERS];
void (*callbacks[NUM_OF_TIMERS])(int);


int32_t elapsed = MAX_TIME_MS;
int32_t next = MAX_TIME_MS;

uint8_t current = 0;
uint8_t expired = 0;

uint8_t flag = 0;
int timer_expired(){
	#ifdef DEBUG 1
	if (flag) {
		Serial.println("Timer expired");
		Serial.print("Timer0: "); Serial.println(timers[0]);
		Serial.print("Next timer : "); Serial.println(next);
		flag = 0;
	}
	#endif
	return expired;
}

void timer_ISR(){
	current = 0;
	next = MAX_TIME_MS;
	for (int i = 0; i < NUM_OF_TIMERS; i++){
		if (timers[i] > 0){ // Check if a valid timer or already timed-out
			timers[i] = timers[i] - elapsed;
			if (timers[i] == 0){
				expired++;
			}
			if (timers[i] > 0 && timers[i] < next){
				next = timers[i];
			}
		}
	}
	#ifdef DEBUG 1
	if (expired==0){
		flag = 1;
	}
	#endif
	//set timer to next shortest timer expiry
	elapsed = next;
	Timer1.setPeriod(next);
}


void init_timer(){
	pinMode(4, OUTPUT);
	for (int i = 0; i < NUM_OF_TIMERS; i++){
		timers[i] = -1;
	}
	Timer1.initialize(MAX_TIME_MS);
	Timer1.attachInterrupt(timer_ISR);
}


int set_timer(double timer, int val, void (*callback)(int)){
	for (int i = 0; i < NUM_OF_TIMERS; i++){
		if (timers[i] == -1){
			timers[i] = timer * ONE_SECOND;
			events[i] = val;
			callbacks[i] = callback;
			if (timers[i] < next){ 
				//Check if less than current next timer
				next = timers[i];
				elapsed = next;
				Timer1.setPeriod(timers[i]);
			}
			return i;
		}
	}//No space in queue
	return -1;
}

void remove_timer(int id){
	if (id < NUM_OF_TIMERS){
		timers[id] = -1;
	}
}


int run_next_expired(){
	cli();
	for (;current < NUM_OF_TIMERS; current++){
		if (timers[current] == 0){
			timers[current] = -1; // Reset to invalidate timer
			expired--;
			sei();
			callbacks[current](events[current]);
			return events[current];
		}
	}
	expired = 0;
	sei();
	return 0;
}

void timer_handler(){
	int chan = 1;
	while (chan){
		chan = run_next_expired();
	}
}