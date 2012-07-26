#ifndef __PRIORITY_QUEUE_H__
#define __PRIORITY_QUEUE_H__

#include <stdbool.h>

//Implement priority queue using a binary heap

typedef struct {
	void **elems;
	unsigned num_elems;
	bool (*compare_gt)(void *elem1, void *elem2);
} priority_queue;

priority_queue *priority_queue_create(bool (*compare_lt)(void *elem1, void *elem2));
void priority_queue_delete(priority_queue *queue);
bool priority_queue_push(priority_queue *queue, void *elem);
void *priority_queue_pull(priority_queue *queue);

#endif