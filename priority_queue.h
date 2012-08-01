#ifndef __PRIORITY_QUEUE_H__
#define __PRIORITY_QUEUE_H__

#include <stdbool.h>

//Implement priority queue using a binary heap

typedef struct {
	void **elems;
	unsigned num_elems;
	bool (*compare_gt)(void *elem1, void *elem2);
	void (*delete)(void *elem);
} priority_queue;

priority_queue *priority_queue_create(bool (*compare_gt)(void *elem1, void *elem2),
									  void (*delete)(void *elem));
void priority_queue_delete(priority_queue *queue);
unsigned priority_queue_num_elems(priority_queue *queue);
bool priority_queue_push(priority_queue *queue, void *elem);
void *priority_queue_pull(priority_queue *queue);
void priority_queue_test(void);


#endif