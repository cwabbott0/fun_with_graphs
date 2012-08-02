#ifndef GRAPH_H

#include "nauty.h"

//Represents when there is no connection
//graph.c adds stuff to infinity (crazy, I know),
//So make sure it won't overflow
#define GRAPH_INFINITY ((int)1000000)
//#define MAXN 1000
#define MAX_K 3
#define P 10000


typedef struct {
	int n;
	int *distances;
	int sum_of_distances;
	int m;
	int *k;
	int diameter;
	int max_k;
	graph *nauty_graph;
} graph_info;

graph_info *graph_info_from_nauty(graph *g, int n);
void graph_info_destroy(graph_info *g);
void floyd_warshall(graph_info g);
void fill_dist_matrix(graph_info g);
void add_edges_and_transfer_to_queue(graph_info input);
void print_graph(graph_info g);
void test_add_edges(void);


#define GRAPH_H
#endif //GRAPH_H
