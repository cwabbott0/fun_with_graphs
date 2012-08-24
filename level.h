#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "graph.h"
#include "hash_set.h"
#include "priority_queue.h"

typedef struct {
	unsigned min_m; // minimum m (n - 1)
	unsigned num_m; // number of possible values of m
	unsigned n;
	unsigned p;
	unsigned max_k;
	
	hash_set **sets; //one hash set for each m
	priority_queue **queues;
	
	int *max_graphs; //for passing from the master to the slaves
} level;

level *level_create(unsigned n, unsigned p, unsigned max_k);
void level_delete(level *my_level);
void level_empty_and_print(level *my_level);
void level_extend(level *old, level *new);
void extend_graph_and_add_to_level(graph_info input, level *new_level);
bool add_graph_to_level(graph_info *new_graph, level *my_level);
void _add_graph_to_level(graph_info *new_graph, level *my_level);
bool level_empty(level *my_level);
void test_extend_graph(void);


#endif