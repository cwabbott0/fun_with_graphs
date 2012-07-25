#ifndef GRAPH_H


//Represents when there is no connection
//graph.c adds stuff to infinity (crazy, I know),
//So make sure it won't overflow
#define GRAPH_INFINITY ((int)1000000)

typedef struct {
	int *distances; //INFINITY for no connection
	int n; //number of vertices
} distance_matrix;

void floyd_warshall(distance_matrix g);
void fill_dist_matrix(distance_matrix g);

#define GRAPH_H
#endif //GRAPH_H
