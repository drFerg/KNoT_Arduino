#ifndef CALLBACK_TIMER_H
#define CALLBACK_TIMER_H
#include <TimerOne.h>

/*Initialises the timer */
void init_timer();

/*
 * Adds a timer to the timer queue.
 * 
 * Parameters: timer specifies the timer duration,
 *             val is passed to the callback function upon timer expiry 
 *             and call to run_next_expired()
 * Returns 0 if queue is full, 1 otherwise. */
int set_timer(double timer, int val, void(*callback)(int));

/* Returns the number of timers that have expired, 0 otherwise */
int timer_expired();

/* Runs the callback function for the next expired timer */
int run_next_expired();

/* Runs the callback function for all expired timers */
void run_all_expired();

/* removes the timer referenced by the id */
void remove_timer(int id);

#endif /* CALLBACK_TIMER_H */