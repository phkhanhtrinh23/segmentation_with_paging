#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>
#include <stdint.h>

struct timer_id_t {
	int done;
	int fsh;
	pthread_cond_t event_cond;
	pthread_mutex_t event_lock;
	pthread_cond_t timer_cond;
	pthread_mutex_t timer_lock;
};

void start_timer();

void stop_timer();

struct timer_id_t * attach_event();

void detach_event(struct timer_id_t * event);

void next_slot(struct timer_id_t* timer_id);

uint64_t current_time();

#endif
