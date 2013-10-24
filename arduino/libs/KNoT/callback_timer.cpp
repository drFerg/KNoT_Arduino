#include "callback_timer.h"
#include <Arduino.h>
#include <TimerOne.h>

#define UNSET_TIMER -1
#define NUM_OF_TIMERS 10
#define MAX_TIME_S 8 //Timer limit is 8.3....s
#define ONE_SECOND 1000000 //One second in microseconds
#define MAX_TIME_MICRO_S (MAX_TIME_S * ONE_SECOND)

#define enable_interrupts sei()
#define disable_interrupts cli()

#define has_timer_expired(time) (time == 0)
#define has_time_left(time) (time > 0)
#define is_timer_free(time) (time == -1)

int32_t timers[NUM_OF_TIMERS];
int events[NUM_OF_TIMERS];
void (*callbacks[NUM_OF_TIMERS])(int); /* array of callback func* */

int32_t elapsed = MAX_TIME_MICRO_S;
int32_t next = MAX_TIME_MICRO_S;

uint8_t current_iter = 0; /* iterator for current expired */
uint8_t expired = 0;

int timer_expired() {
	return expired;
}

void timer_ISR() {
	current_iter = 0;
	next_timer = MAX_TIME_MICRO_S;
	for (int i = 0; i < NUM_OF_TIMERS; i++) {
		if (has_time_left(timers[i])){ /* Check if a valid timer */
			timers[i] = timers[i] - elapsed; /* Decrement time elapsed */
			if (has_timer_expired(timers[i])) { /* Check if now expired */
				expired++;
			}
			else if (timers[i] < next_timer) { /* find next closest time */
				next_timer = timers[i];
			}
		}
	}
	elapsed = next_timer; /* Set timer to next shortest timer expiry */
	Timer1.setPeriod(next_timer);
}


void init_timer() {
	pinMode(4, OUTPUT);
	for (int i = 0; i < NUM_OF_TIMERS; i++) {
		timers[i] = -1;
	}
	Timer1.initialize(MAX_TIME_MICRO_S);
	Timer1.attachInterrupt(timer_ISR);
}


int set_timer(double timer, int val, void (*callback)(int)) {
	for (int i = 0; i < NUM_OF_TIMERS; i++) {
		if (is_timer_free(timers[i])){
			timers[i] = timer * ONE_SECOND; /* Timer is in microseconds */
			events[i] = val;
			callbacks[i] = callback;
			if (timers[i] < next_timer) { 
				next_timer = timers[i]; /* Check if less than current_iter next timer */
				elapsed = next_timer;
				Timer1.setPeriod(timers[i]);
			}
			return i;
		}
	}
	return -1; /* No space in queue */
}

void remove_timer(int timer_id) {
	if (timer_id < NUM_OF_TIMERS) {
		timers[timer_id] = UNSET_TIMER;
	}
}


int run_next_expired() {
	disable_interrupts();
	for (;current_iter < NUM_OF_TIMERS; current_iter++) {
		if (has_timer_expired(timers[current_iter])) {
			timers[current_iter] = UNSET_TIMER; // Reset to invalidate timer
			expired--;
			enable_interrupts();
			/* Run callback function */
			callbacks[current_iter](events[current_iter]); 
			return events[current_iter];
		}
	}
	expired = 0;
	disable_interrupts();
	return 0;
}

void timer_handler() {
	int channels_still_left = 1;
	while (channels_still_left) {
		channels_still_left = run_next_expired();
	}
}