#include "graph.h"
#include "hash_set.h"
#include <stdio.h>

int main(void)
{
	distance_matrix g;
	int distances [25] = {
		GRAPH_INFINITY, 1, 1, 2, 2,
		1, GRAPH_INFINITY, 1, 2, 1,
		1, 1, GRAPH_INFINITY, 1, 2,
		2, 2, 1, GRAPH_INFINITY, 1,
		2, 1, 2, 1, GRAPH_INFINITY,
	};
	g.distances = distances;
	g.n = 5;
	int g_k[5] = {2, 3, 3, 2 ,2};
	g.k = g_k;
	g.m = 6;
	
	add_edges_and_transfer_to_queue(g);
	
	return 0;
}

void geng_callback(FILE *file, graph *g, int n)
{
	
}