#include "callback_timer.h"
#include <Arduino.h>
#include <TimerOne.h>

/* Useful values macros */
#define UNSET_TIMER -1
#define NUM_OF_TIMERS 10
#define MAX_TIME_S 8 /* Timer limit is 8.3....s */
#define ONE_SECOND 1000000 /* One second in microseconds */
#define MAX_TIME_MICRO_S (MAX_TIME_S * ONE_SECOND)

/* Helper function macros */
#define enable_interrupts sei()
#define disable_interrupts cli()
#define has_timer_expired(time) (time == 0)
#define has_time_left(time) (time > 0)
#define is_timer_free(time) (time == -1)

static int32_t timers[NUM_OF_TIMERS]; /* Array of timers */
static int events[NUM_OF_TIMERS]; /* Array of values to pass to functions */
static void (*callbacks[NUM_OF_TIMERS])(int); /* Array of callback func* */

static int32_t elapsed = MAX_TIME_MICRO_S; /* Time elapsed since last timer set */
static int32_t next = MAX_TIME_MICRO_S; /* Next time till expiry */

static uint8_t current_iter = 0; /* Iterator for current expired */
static uint8_t expired_timers = 0; /* Count of timers that have expired */

/* Interrupt service routine for timer expiry */
void timer_ISR() {
	current_iter = 0;
	next_timer = MAX_TIME_MICRO_S;
	for (int i = 0; i < NUM_OF_TIMERS; i++) {
		if (has_time_left(timers[i])){ /* Check if a valid timer */
			timers[i] = timers[i] - elapsed; /* Decrement time elapsed */
			if (has_timer_expired(timers[i])) { /* Check if now expired */
				expired_timers++;
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
	pinMode(4, OUTPUT); /* Pin 4 is used for timer interrupt */
	for (int i = 0; i < NUM_OF_TIMERS; i++) {
		timers[i] = UNSET_TIMER;
	}
	Timer1.initialize(MAX_TIME_MICRO_S);
	Timer1.attachInterrupt(timer_ISR);
}

int set_timer(double timer, int func_value, void (*callback)(int)) {
	for (int i = 0; i < NUM_OF_TIMERS; i++) {
		if (is_timer_free(timers[i])){
			timers[i] = timer * ONE_SECOND; /* Timer is in microseconds */
			events[i] = func_value; /* Store value to pass to callback func */
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

int timer_expired() {
	return expired_timers;
}

int run_next_expired() {
	disable_interrupts();
	for (;current_iter < NUM_OF_TIMERS; current_iter++) {
		if (has_timer_expired(timers[current_iter])) {
			timers[current_iter] = UNSET_TIMER; // Reset to invalidate timer
			expired_timers--;
			enable_interrupts();
			/* Run callback function */
			callbacks[current_iter](events[current_iter]); 
			return 1;
		}
	}
	enable_interrupts();
	return 0;
}

void run_all_expired() {
	while (expired_timers) {
		run_next_expired();
	}
}