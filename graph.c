#include "graph.h"


void floyd_warshall(distance_matrix g) {
	for (int k = 0; k < g.n; k++) {
		for (int i = 0; i < g.n; i++) {
			for (int j = 0; j < g.n; j++) {
				int dist = g.distances[g.n*i + k] + g.distances[g.n*k + j];
				if(dist < g.distances[g.n*i + j]) {
					g.distances[g.n*i + j] = dist;
				}
			}
		}
	}
}

void fill_dist_matrix(distance_matrix g)
{
	//Figure out distance from new node to each other node
	for(int i = 0; i < g.n-1; i++)
	{
		if(g.distances[g.n*i + g.n-1] == GRAPH_INFINITY)
		{
			int min_dist = GRAPH_INFINITY;
			for(int j = 0; j < g.n-1; j++)
				if(g.distances[g.n*(g.n-1) + j] == 1 
				   && g.distances[g.n*j + i] + 1 < min_dist)
					 min_dist = g.distances[g.n*j + i] + 1;
			g.distances[g.n*(g.n-1) + i] = g.distances[g.n*i + g.n-1] = min_dist;
		}
	}
	
	//One iteration of Floyd-Warshall with k = g.n - 1
	for (int i = 0; i < g.n; i++) {
		for (int j = 0; j < g.n; j++) {
			int dist = g.distances[g.n*i + g.n-1] + g.distances[g.n*g.n-1 + j];
			if(dist < g.distances[g.n*i + j]) {
				g.distances[g.n*i + j] = dist;
			}
		}
	}
		
}

void test_fill_dist_matrix(void)
{
	distance_matrix g;
	int distances[9] = {
		GRAPH_INFINITY, 1, 1,
		1, GRAPH_INFINITY, GRAPH_INFINITY,
		1, GRAPH_INFINITY, GRAPH_INFINITY
	};
	g.distances = distances;
	g.n = 3;
	
	fill_dist_matrix(g);
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
			printf("%d, ", g.distances[i*3 + j]);
		printf("\n");
	}
}

void extend_dist_matrix(distance_matrix g)
{
	distance_matrix extended;
	extended.n = g.n + 1;
	extended.distances = (int*) malloc((extended.n)*(extended.n)*sizeof(int));
	for (int i = 0; i < (extended.n)*(extended.n); i++)
		extended.distances[i] = GRAPH_INFINITY;
	for (int i = 0; i < (g.n*g.n); i++)
		extended.distances[(i % g.n) + ((i/g.n)*extended.n)] = g.distances[i];
}