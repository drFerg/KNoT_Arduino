#ifndef RESPONSE_TIMER_H
#define RESPONSE_TIMER_H

#include <TimerOne.h>

void init_timer();
int set_timer(int timer, int val, void(*callback)(int));
int get_next_expired();
int run_next_expired();

int timer_expired();
int is_timer_event();

#endif /* RESPONSE_TIMER_H */