#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */
	if (q->size >= MAX_QUEUE_SIZE) return;	
	size_t i;
	for (i=0; i<q->size; i++){
		if (proc->priority < q->proc[i]->priority) break;
	}
	for (size_t j=q->size; j>i; j--){
		q->proc[j] = q->proc[j-1];
	}
	q->proc[i] = proc;
	q->size++;
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if (empty(q) == 1){
		return NULL;
	}
	else{
		q->size--;
		return q->proc[q->size];
	}
}

