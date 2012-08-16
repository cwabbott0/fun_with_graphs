#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "graph.h"
#include "hash_set.h"
#include "priority_queue.h"
#include <mpi.h>

#define NEW_LEVEL 0
#define SLAVE_KILL 1
#define SLAVE_INPUT 2
#define SLAVE_OUTPUT 3
#define SLAVE_REQUEST 4

#ifdef SETWORD_SHORT
#define MPI_SETWORD MPI_UNSIGNED_SHORT
#else 
#ifdef SETWORD_INT
#define MPI_SETWORD MPI_UNSIGNED
#else
#ifdef SETWORD_LONG
#define MPI_SETWORD MPI_UNSIGNED_LONG
#else
#define MPI_SETWORD MPI_UNSIGNED_LONG_LONG
#endif
#endif
#endif

typedef struct {
	unsigned min_m; // minimum m (n - 1)
	unsigned num_m; // number of possible values of m
	unsigned n;
	unsigned p;
	unsigned max_k;
	
	hash_set **sets; //one hash set for each m
	priority_queue **queues;
} level;

level *level_create(unsigned n, unsigned p, unsigned max_k);
void level_delete(level *my_level);
void level_empty_and_print(level *my_level);
void level_extend(level *old, level *new);
void extend_graph_and_add_to_level(graph_info input, level *new_level);
bool add_graph_to_level(graph_info *new_graph, graph *gcan, level *my_level);
void _add_graph_to_level(graph_info *new_graph, level *my_level);
void test_extend_graph(void);
void init_extended(graph_info input, graph_info *extended);
void add_edges(graph_info *g, unsigned start, int extended_m, int rank, int n);

graph_info* receive_graph(int tag, int src, int n);
void send_graph(int tag, int dest, graph_info* graph);

#endif
