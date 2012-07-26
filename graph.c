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

distance_matrix add_extra_node(distance_matrix input)
{
	distance_matrix output;
	output.n = (input.n + 1);
	output.distances = malloc(n*n*sizeof(*(out.distances)));
	for(int i = 0; i < input.n; i++)
		for(int j = 0; j < input.n; j++)
		output.distances[(output.n)*i + j)] = input.distances[(input.n)*i + j)];
	for(int i = 0; i  output n; i++)
	output.distances[(output.n)*i + output.n - 1] = output.distances[(output.n)*(output.n - 1) + i] = 1;
	return output
}

int Binomial[n][n*(n-1)/2];
void put_into_queue(distance_matrix g, int m)
void next_combination(distance_matrix g, distance_matrix replacement);

void add_edges_and_transfer_to_queue(distance_matrix input, int original_edges, int edges_added)
{
int total edges = original_edges + edges_added;
distance_matrix submatrix;
submatrix.n = input.n-1;
submatrix.distances = malloc(submatrix.n*submatrix.n*sizeof(*(out.distances)));
for(int i = 0; i < submatrix.n; i++)
for(int j = 0; j < submatrix.n; j++)
submatrix.distances[input.n*i+j] = input.distances[input.n*i+j]

for(int i = 0; i < edges_added; i++)
input.distances[input.n*(input.n-1)+i] = input.distances[input.n*i + input.n - 1] = 1;

int count = 0;
while(count < Binomial[input.n - 1][edges_added])
{
fill_dist_matrix(input.n);
put_into_queue(input,total_edges);
next_combination(input,submatrix);
count++;
}
}

void next_combination(distance_matrix g, distance_matrix replacement)
{
for(int i = 0; i < g.n; i++)
if((g.distances[g.n*i+g.n-1] == 1) && (g.distances[g.n*(i+1)+g.n-1] != 1))
{int previous_edges = 0;
for(int j = 0; j < i; j++)
if(g.distances[g.n*j + g.n-1] == 1)
previous_edges++;

for(int j = 0; j < previous_edges; j++)
g.distances[g.n*j + g.n - 1] = g.distances[g.n*(g.n-1) + j] = 1;
for(int j = previous_edges; j <= i; j++)
input.distances[g.n*j + g.n - 1] = g.distances[g.n*(g.n-1) + j] = GRAPH_INIFINITY;
input.distances[g.n*(i+1) + g.n-1] = g.distances[g.n*(g.n-1) + i + 1] = 1;
for(int j = i+2; j < g.n - 1; j++)
if(g.distances[g.n*j + g.n-1] != 1)
g.distances[g.n*j + g.n-1] = g.distances[g.n*(g.n-1) + j] = GRAPH_INFINITY;

for(int i = 0; i < replacement.n; i++)
for(int j = 0; j < replacement.n; j++)
g.distances[g.n*i+j] = replacement.distances[replacement.n*i+j];
break;
}
}

