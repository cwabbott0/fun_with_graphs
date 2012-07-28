#ifndef GRAPH_H

#include "nauty.h"

//Represents when there is no connection
//graph.c adds stuff to infinity (crazy, I know),
//So make sure it won't overflow
#define GRAPH_INFINITY ((int)1000000)
//#define MAXN 1000

typedef struct {
	int *distances; //INFINITY for no connection
	int n; //number of vertices
} distance_matrix;

typedef struct {
	distance_matrix matrix;
	int sum_of_distances;
	int m;
	int *k;
	int diameter;
} graph_info;

void floyd_warshall(distance_matrix g);
void fill_dist_matrix(distance_matrix g);

#define GRAPH_H
#endif //GRAPH_H
