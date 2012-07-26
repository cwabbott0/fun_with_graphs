#include "graph.h"
#include "priority_queue.h"
#include <stdio.h>

bool compare_gt(void *arg1, void *arg2)
{
	return *((int*)arg1) > *((int*)arg2);
}

int main(void)
{
	priority_queue *queue = priority_queue_create(compare_gt);
	int ints[5] = {3, 5, -1, 2, 10};
	for(int i = 0; i < 5; i++)
		priority_queue_push(queue, ints + i);
	for(int i = 0; i < 5; i++)
	{
		int *val = priority_queue_pull(queue);
		printf("%d, ", *val);
	}
	priority_queue_delete(queue);
	return 0;
}

void geng_callback(FILE *file, graph *g, int n)
{
	
}