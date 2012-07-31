#ifndef GRAPH_H

#include "nauty.h"

//Represents when there is no connection
//graph.c adds stuff to infinity (crazy, I know),
//So make sure it won't overflow
#define GRAPH_INFINITY ((int)1000000)
//#define MAXN 1000
#define MAX_K ((int)4)
#define MAX_N ((int)10)


typedef struct {
	int n;
	int *distances;
	int sum_of_distances;
	int m;
	int *k;
	int diameter;
	int max_k;
} distance_matrix;

void floyd_warshall(distance_matrix g);
void fill_dist_matrix(distance_matrix g);
void add_edges_and_transfer_to_queue(distance_matrix input);


#define GRAPH_H
#endif //GRAPH_H
