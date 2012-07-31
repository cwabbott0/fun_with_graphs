#include "graph.h"
#include <stdbool.h>

void print(distance_matrix g)
{
	for (int i = 0; i < g.n; i++)
	{
		for (int j = 0; j < g.n; j++)
			printf("%d\t", g.distances[g.n*i + j]);
		printf("\n");
	}

	for (int i = 0; i < g.n; i++)
		printf("%d ", g.k[i]);
	printf("\n");
	printf("K: %d, D: %d, S: %d\n", g.max_k, g.diameter, g.sum_of_distances);
}


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
	for (int i = 0; i < g.n-1; i++) {
		for (int j = i+1; j < g.n-1; j++) {
			int dist = g.distances[g.n*i + g.n-1] + g.distances[g.n*(g.n-1) + j];
			if(dist < g.distances[g.n*i + j]) {
				g.distances[g.n*i + j] = g.distances[g.n*j+i] = dist;
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

int calc_sum(distance_matrix g)
{
	int sum = 0;
	for(int i = 0; i < g.n; i++)
		for(int j = i+1; j < g.n; j++)
		sum += g.distances[g.n*i+j];
	return sum;
}

int calc_diameter(distance_matrix g)
{
	int diameter = 0;
	for(int i = 0; i < g.n; i++)
		for(int j = i+1; j < g.n; j++)
			if(diameter < g.distances[g.n*i + j])
				diameter = g.distances[g.n*i + j];
	return diameter;
}


static void init_extended(distance_matrix input, distance_matrix *extended)
{
	extended->n = (input.n+1);

	extended->distances = malloc(extended->n*extended->n*sizeof(*extended->distances));
	for(int i = 0; i < input.n; i++)
		for(int j = 0; j < input.n; j++)
			extended->distances[(extended->n)*i + j] = input.distances[(input.n)*i + j];
	for(int i = 0; i < extended->n; i++)
		extended->distances[(extended->n)*i+extended->n-1] =
		extended->distances[(extended->n)*(extended->n-1)+i] = GRAPH_INFINITY;

	extended->k = (int*) malloc(extended->n*sizeof(int));
	for(int i = 0; i < input.n; i++)
		extended->k[i] = input.k[i];
	extended->k[input.n] = 0;

	extended->m = input.m;
	extended->max_k = input.max_k;
}

static void add_edges(distance_matrix g, unsigned start)
{
	g.m++;
	g.k[g.n - 1]++;
	unsigned old_max_k = g.max_k;
	if(g.k[g.n - 1] > g.max_k)
		g.max_k = g.k[g.n - 1];
	
	if(g.k[g.n - 1] <= MAX_K)
	{
		for(unsigned i = start; i < g.n - 1; i++)
		{
			g.k[i]++;
			if(g.k[i] <= MAX_K)
			{
				unsigned old_max_k = g.max_k;
				if(g.k[i] > g.max_k)
					g.max_k = g.k[i];
				
				g.distances[g.n*i + (g.n-1)] = g.distances[g.n*(g.n-1) + i] = 1;
				
				add_edges(g, i + 1);
				
				g.distances[g.n*i + (g.n-1)] = g.distances[g.n*(g.n-1) + i] = GRAPH_INFINITY;
				g.max_k = old_max_k;
			}
			g.k[i]--;
		}
	}
	
	g.max_k = old_max_k;
	g.m--;
	g.k[g.n - 1]--;
	g.max_k = old_max_k;
	
	if(g.k[g.n - 1] > 0)
		print(g);
}

void add_edges_and_transfer_to_queue(distance_matrix input)
{
	distance_matrix extended;
	init_extended(input, &extended);
	
	add_edges(extended, 0);
}
