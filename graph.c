#include "graph.h"
#include <stdbool.h>

void print_graph(graph_info g)
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

	unsigned m = (g.n + WORDSIZE - 1) / WORDSIZE;
	for(int i = 0; i < g.n; i++)
	{
		for(int j = 0; j < g.n; j++)
		{
			if(ISELEMENT(GRAPHROW(g.nauty_graph, i, m), j))
				printf("1, ");
			else
				printf("0, ");
		}
		printf("\n");
	}
	printf("\n");
	printf("K: %d, D: %d, S: %d, M: %d\n", g.max_k, g.diameter, g.sum_of_distances, g.m);
}


void floyd_warshall(graph_info g) {
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

void fill_dist_matrix(graph_info g)
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
	graph_info g;
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

int calc_sum(graph_info g)
{
	int sum = 0;
	for(int i = 0; i < g.n; i++)
		for(int j = i+1; j < g.n; j++)
		sum += g.distances[g.n*i+j];
	return sum;
}

int calc_diameter(graph_info g)
{
	int diameter = 0;
	for(int i = 0; i < g.n; i++)
		for(int j = i+1; j < g.n; j++)
			if(diameter < g.distances[g.n*i + j])
				diameter = g.distances[g.n*i + j];
	return diameter;
}

graph_info *graph_info_from_nauty(graph *g, int n)
{
	graph_info *ret = malloc(sizeof(graph_info));
	ret->n = n;
	ret->distances = malloc(n * n * sizeof(*ret->distances));
	ret->k = malloc(n * sizeof(*ret->k));

	int m = (n + WORDSIZE - 1) / WORDSIZE;
	ret->m = 0; //total number of edges
	for (int i = 0; i < n; i++) {
		ret->k[i] = 0;
		for (int j = 0; j < n; j++) {
			if(i == j)
				ret->distances[n*i + j] = 0;
			else if(ISELEMENT(g + i*m, j))
			{
				ret->distances[n*i + j] = 1;
				ret->k[i]++;
				ret->m++;
			}
			else
				ret->distances[n*i + j] = GRAPH_INFINITY;
		}
	}
	ret->m /= 2;
	
	ret->max_k = 0;
	for (int i = 0; i < n; i++)
		if (ret->k[i] > ret->max_k)
			ret->max_k = ret->k[i];
	
	floyd_warshall(*ret);
	ret->sum_of_distances = calc_sum(*ret);
	ret->diameter = calc_diameter(*ret);
	ret->nauty_graph = malloc(n * m * sizeof(graph));
	memcpy(ret->nauty_graph, g, n * m * sizeof(graph));
	
	return ret;
}

void graph_info_destroy(graph_info *g)
{
	free(g->distances);
	free(g->k);
	free(g->nauty_graph);
	free(g);
}

graph_info *new_graph_info(graph_info src)
{
	graph_info *ret = (graph_info*) malloc(sizeof(graph_info));
	int m = (src.n + WORDSIZE - 1) / WORDSIZE;
	ret->n = src.n;
	ret->distances = malloc(ret->n * ret->n * sizeof(*ret->distances));
	ret->nauty_graph = malloc(ret->n * m * sizeof(setword));
	ret->k = malloc(ret->n * sizeof(*ret->k));
	ret->k = src.k;
	ret->m = src.m;
	ret->max_k = src.max_k;
	memcpy(ret->distances, src.distances, src.n * src.n * sizeof(*src.distances));
	memcpy(ret->nauty_graph, src.nauty_graph, src.n * m * sizeof(setword));
	return ret;
}