#include "priority_queue.h"
#include <stdlib.h>

priority_queue *priority_queue_create(bool (*compare_gt)(void *elem1, void *elem2))
{
	priority_queue *queue = malloc(sizeof(priority_queue));
	queue->elems = NULL;
	queue->num_elems = 0;
	queue->compare_gt = compare_gt;
	return queue;
}

void priority_queue_delete(priority_queue *queue)
{
	if(queue->elems)
		free(queue->elems);
	free(queue);
}

unsigned priority_queue_num_elems(priority_queue *queue)
{
	return queue->num_elems;
}

static bool is_power_of_two(unsigned num)
{
	return !(num & (num - 1));
}

bool priority_queue_push(priority_queue *queue, void *elem)
{
	if(queue->num_elems == 0)
	{
		queue->num_elems = 1;
		queue->elems = malloc(sizeof(void*));
		if(!queue->elems)
			return false;
		queue->elems[0] = elem;
		return true;
	}
	
	if(is_power_of_two(queue->num_elems))
	{
		queue->elems = realloc(queue->elems, queue->num_elems * 2 *sizeof(void*));
		if(!queue->elems)
			return false;
	}
	
	queue->elems[queue->num_elems] = elem;
	
	unsigned cur_elem = queue->num_elems;
	while(cur_elem != 0)
	{
		unsigned parent = (cur_elem - 1) / 2;
		if(!queue->compare_gt(queue->elems[cur_elem], queue->elems[parent]))
			break;
		
		void *temp = queue->elems[parent];
		queue->elems[parent] = queue->elems[cur_elem];
		queue->elems[cur_elem] = temp;
		cur_elem = parent;
	}
	
	queue->num_elems++;
}

void *priority_queue_pull(priority_queue *queue)
{
	if(queue->num_elems == 0)
		return NULL;
	
	void *ret = queue->elems[0];
	
	if(queue->num_elems == 1)
	{
		free(queue->elems);
		queue->elems = NULL;
		return ret;
	}
	
	queue->elems[0] = queue->elems[queue->num_elems-1];
	
	unsigned cur_elem = 0;
	while (true)
	{
		unsigned smallest = cur_elem;
		unsigned left = 2*cur_elem + 1;
		unsigned right = 2*cur_elem + 2;
		if(left < queue->num_elems - 1 &&
		   queue->compare_gt(queue->elems[left], queue->elems[smallest]))
		   smallest = left;
		if(right < queue->num_elems - 1 &&
		   queue->compare_gt(queue->elems[right], queue->elems[smallest]))
		   smallest = right;
		
		if(smallest == cur_elem)
			break;
		
		void *temp = queue->elems[smallest];
		queue->elems[smallest] = queue->elems[cur_elem];
		queue->elems[cur_elem] = temp;
		cur_elem = smallest;
	}
	
	if(is_power_of_two(queue->num_elems))
	{
		queue->elems = realloc(queue->elems, queue->num_elems / 2);
		if(!queue->elems)
			return NULL;
	}
	
	queue->num_elems--;
	return ret;
}