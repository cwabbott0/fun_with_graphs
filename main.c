#include "graph.h"
#include "hash_set.h"
#include <stdio.h>

unsigned long hash(void *elem)
{
	return *((unsigned*)elem);
}

bool compare(void *arg1, void *arg2)
{
	return *((int*)arg1) == *((int*)arg2);
}

void delete(void *elem)
{
}

int main(void)
{
	unsigned ints[3] = {0, 10, 15};
	hash_set *set = hash_set_create(10, hash, compare, delete);
	for(int i = 0; i < 3; i++)
		hash_set_add(set, ints + i);
	
	for(int i = 0; i < 3; i++)
	{
		unsigned num = ints[i];
		if(!hash_set_contains(set, &num))
			printf("Error: %d returns false for hash_set_contains\n", num);
	}
	
	hash_set_delete(set);
}

void geng_callback(FILE *file, graph *g, int n)
{
	
}