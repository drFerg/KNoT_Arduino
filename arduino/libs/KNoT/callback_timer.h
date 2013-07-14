#ifndef CALLBACK_TIMER_H
#define CALLBACK_TIMER_H

#include <TimerOne.h>
/*Initialises the timer */
void init_timer();

/*Adds a timer to the timer queue.
 * val is passed to the callback function upon timer expiry 
 * and call to run_next_expired()
 * - Returns 0 if queue is full, 1 otherwise. */
int set_timer(int timer, int val, void(*callback)(int));

/* Runs the next expired timer's callback function */
int run_next_expired();

/* Returns the number of timers that have expired, 0 otherwise */
int timer_expired();

#endif /* CALLBACK_TIMER_H */