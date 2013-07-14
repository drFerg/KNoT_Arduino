#include "callback_timer.h"
#include <Arduino.h>
#include <TimerOne.h>


#define NUM_OF_TIMERS 10
#define MAX_TIME 8 //Timer limit is 8.3....s
#define ONE_SECOND 1000000 //One second in microseconds

int timers[NUM_OF_TIMERS];
int events[NUM_OF_TIMERS];
void (*callbacks[NUM_OF_TIMERS])(int);


int elapsed = 1;
int current = 0;
int next = 8;
int expired = 0;

int timer_expired(){
	return expired;
}

void timer_ISR(){
	current = 0;
	next = MAX_TIME;
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

	//set timer to next shortest timer expiry
	elapsed = next;
	Timer1.setPeriod(next * ONE_SECOND);
}


void init_timer(){
	pinMode(4, OUTPUT);
	for (int i = 0; i < NUM_OF_TIMERS; i++){
		timers[i] = -1;
	}
	Timer1.initialize(MAX_TIME * ONE_SECOND);
	Timer1.attachInterrupt(timer_ISR);
}


int set_timer(int timer, int val, void (*callback)(int)){
	for (int i = 0; i < NUM_OF_TIMERS; i++){
		if (timers[i] == -1){
			timers[i] = timer;
			events[i] = val;
			callbacks[i] = callback;
			if (timer < next){ 
				//Check if less than current next timer
				next = timer;
				elapsed = next;
				Timer1.setPeriod(timer * ONE_SECOND);
			}
			return i;
		}
	}
	return -1;
}

void remove_timer(int index){
	if (index < NUM_OF_TIMERS){
		timers[index] = -1;
	}
}


int run_next_expired(){
	cli();
	for (;current < NUM_OF_TIMERS; current++){
		if (timers[current] == 0){
			timers[current] = -1; // Reset to invalidate timer
			expired--;
			sei();

			return events[current];
		}
	}
	expired = 0;
	sei();
	return 0;
}