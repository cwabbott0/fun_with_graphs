#ifndef GRAPH_H


#include "nauty.h"

//Represents when there is no connection
//graph.c adds stuff to infinity (crazy, I know),
//So make sure it won't overflow
#define GRAPH_INFINITY ((int)1000000)
//#define MAXN 1000
//Note that if you change this you must change graph_sizes[]
//(See main.c)
#define MAX_K 3
#define P 500


typedef struct {
	int n;
	int *distances;
	int sum_of_distances;
	int m;
	int *k;
	int diameter;
	int max_k;
	graph *nauty_graph, *gcan;
} graph_info;

graph_info *new_graph_info(graph_info src);
graph_info *graph_info_from_nauty(graph *g, int n);
void graph_info_destroy(graph_info *g);
void floyd_warshall(graph_info g);
void fill_dist_matrix(graph_info g);
void print_graph(graph_info g);
int calc_sum(graph_info g);
int calc_diameter(graph_info g);
int calc_k(graph_info g);
int calc_max_k(graph_info g);
int calc_m(graph_info g);


#define GRAPH_H
#endif //GRAPH_H
