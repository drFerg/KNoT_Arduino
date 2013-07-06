#include "response_timer.h"
#include <Arduino.h>
#include <TimerOne.h>


#define NUM_OF_TIMERS 10
#define ONE_SECOND 8000000

int timers[NUM_OF_TIMERS];
int events[NUM_OF_TIMERS];


int elapsed = 1;
int current = 0;

int flag = 0;
int expired = 0;
void timerIsr()
{
    // Toggle LED
    digitalWrite( 4, digitalRead( 4 ) ^ 1 );
}



int is_timer_event(){
	int temp = flag;
	flag = 0;
	return temp;
}

int timer_expired(){
	return expired;
}

void timer_ISR(){
	current = 0;
	int next = 8;
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

	
	//set timerIRQ to next timer
	
	
}


void init_timer(){
	pinMode(4, OUTPUT);
	for (int i = 0; i < NUM_OF_TIMERS; i++){
		timers[i] = -1;
	}
	Timer1.initialize(ONE_SECOND);
	Timer1.attachInterrupt(timer_ISR);
}


int set_timer(int timer, int val){
	for (int i = 0; i < NUM_OF_TIMERS; i++){
		if (timers[i] == -1){
			timers[i] = timer;
			events[i] = val;
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
int get_next_expired(){
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