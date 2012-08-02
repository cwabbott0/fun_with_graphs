#include "level.h"
#include "naututil.h"
#include <string.h>

//type for the hash set
//we need to know n so we can compare & hash
typedef struct {
	unsigned n;
	graph *g;
} nauty_graph_with_n;

//Hash set implementation/callbacks

static unsigned long nauty_hash(void *elem)
{
	nauty_graph_with_n *_graph = elem;
	graph *g = _graph->g;
	int m = (_graph->n + WORDSIZE - 1) / WORDSIZE;
	return (unsigned long) hash(g, m * _graph->n, 15);
}

static bool nauty_compare(void *elem1, void *elem2)
{
	nauty_graph_with_n *graph1 = elem1, *graph2 = elem2;
	if (graph1->n != graph2->n)
		return false;
	
	graph *g1 = graph1->g, *g2 = graph2->g;
	int m = (graph1->n + WORDSIZE - 1) / WORDSIZE;
	return !memcmp(g1, g2, graph1->n * m * sizeof(setword));
}

static void nauty_delete(void *elem)
{
	nauty_graph_with_n *graph = elem;
	free(graph->g);
	free(graph);
}

//Priority queue implementation/callbacks

static bool graph_compare_gt(void *elem1, void *elem2)
{
	graph_info *graph1 = elem1, *graph2 = elem2;
	
	if(graph1->sum_of_distances > graph2->sum_of_distances)
		return true;
	else if(graph1->sum_of_distances < graph2->sum_of_distances)
		return false;
	return graph1->diameter > graph2->diameter;
}

static void graph_delete(void *elem)
{
	graph_info *graph = elem;
	graph_info_destroy(graph);
}


level *level_create(unsigned n, unsigned p, unsigned max_k)
{
	level *ret = malloc(sizeof(level));
	ret->n = n;
	ret->p = p;
	ret->max_k = max_k;
	ret->min_m = n - 1;
	ret->num_m = (n * (n - 1) / 2) - ret->min_m;
	
	ret->sets = malloc(ret->num_m * sizeof(hash_set*));
	ret->queues = malloc(ret->num_m * sizeof(priority_queue*));
	
	for(int i = 0; i < ret->num_m; i++)
	{
		ret->sets[i] = hash_set_create(1024, nauty_hash, nauty_compare,
									   nauty_delete);
		ret->queues[i] = priority_queue_create(graph_compare_gt,
											   graph_delete);
	}
	
	return ret;
}

void level_delete(level *my_level)
{
	for(int i = 0; i < my_level->num_m; i++)
		hash_set_delete(my_level->sets[i]);
	free(my_level->sets);
	
	for(int i = 0; i < my_level->num_m; i++)
		priority_queue_delete(my_level->queues[i]);
	free(my_level->queues);
	
	free(my_level);
}

void level_empty_and_print(level *my_level)
{
	printf("For n = %u", my_level->n);
	for(int i = 0; i < my_level->num_m; i++)
	{
		printf("m = %u:", i + my_level->min_m);
		while(priority_queue_num_elems(my_level->queues[i]))
		{
			graph_info *g = priority_queue_pull(my_level->queues[i]);
			print_graph(*g);
			graph_info_destroy(g);
		}
	}
}

static void init_extended(graph_info input, graph_info *extended)
{
	extended->n = (input.n+1);
	
	int m = (input.n + WORDSIZE - 1) / WORDSIZE;
	int extended_m = (input.n + WORDSIZE)/WORDSIZE;
	
	extended->nauty_graph = malloc(extended->n * extended_m * sizeof(setword));
	
	for(int i = 0; i < input.n; i++)
	{
		for(int j = 0; j < m; j++)
			extended->nauty_graph[i*extended_m + j] = input.nauty_graph[i*m + j];
		if(extended_m > m)
			extended->nauty_graph[i*extended_m + m] = 0;
	}
	for(int i = 0; i < extended_m; i++)
		extended->nauty_graph[input.n*extended_m + i] = 0;
	
	extended->distances = malloc(extended->n*extended->n*sizeof(*extended->distances));
	for(int i = 0; i < input.n; i++)
		for(int j = 0; j < input.n; j++)
			extended->distances[(extended->n)*i + j] = input.distances[(input.n)*i + j];
	for(int i = 0; i < extended->n - 1; i++)
		extended->distances[(extended->n)*i+extended->n-1] =
		extended->distances[(extended->n)*(extended->n-1)+i] = GRAPH_INFINITY;
	extended->distances[extended->n*extended->n - 1] = 0;
	
	extended->k = (int*) malloc(extended->n*sizeof(int));
	for(int i = 0; i < input.n; i++)
		extended->k[i] = input.k[i];
	extended->k[input.n] = 0;
	
	extended->m = input.m;
	extended->max_k = input.max_k;
}

static void destroy_extended(graph_info extended)
{
	free(extended.distances);
	free(extended.nauty_graph);
	free(extended.k);
}

static void add_edges(graph_info *g, unsigned start, int extended_m)
{
	//setup m and k[n] for the children
	//note that these values will not change b/w each child
	//of this node in the search tree
	g->m++;
	g->k[g->n - 1]++;
	unsigned old_max_k = g->max_k;
	if(g->k[g->n - 1] > g->max_k)
		g->max_k = g->k[g->n - 1];
	
	//if the child has a node of degree greater than MAX_K,
	//don't search it
	if(g->k[g->n - 1] <= MAX_K)
	{
		for(unsigned i = start; i < g->n - 1; i++)
		{
			g->k[i]++;
			
			//same as comment above
			if(g->k[i] <= MAX_K)
			{
				unsigned old_max_k = g->max_k;
				if(g->k[i] > g->max_k)
					g->max_k = g->k[i];
				
				g->distances[g->n*i + (g->n-1)] = g->distances[g->n*(g->n-1) + i] = 1;
				ADDELEMENT(GRAPHROW(g->nauty_graph, i, extended_m), g->n-1);
				ADDELEMENT(GRAPHROW(g->nauty_graph, g->n-1, extended_m), i);
				
				add_edges(g, i + 1, extended_m);
				
				DELELEMENT(GRAPHROW(g->nauty_graph, i, extended_m), g->n-1);
				DELELEMENT(GRAPHROW(g->nauty_graph, g->n-1, extended_m), i);
				g->distances[g->n*i + (g->n-1)] = g->distances[g->n*(g->n-1) + i] = GRAPH_INFINITY;
				g->max_k = old_max_k;
			}
			g->k[i]--;
		}
	}
	
	//tear down values we created in the beginning
	g->max_k = old_max_k;
	g->m--;
	g->k[g->n - 1]--;
	
	
	if(g->k[g->n - 1] > 0)
	{
		graph_info *temporary = new_graph_info(*g);
		fill_dist_matrix(*temporary); 
		temporary->diameter = calc_diameter(*temporary); 
		temporary->sum_of_distances = calc_sum(*temporary); 
		print_graph(*temporary);
	}
}

void extend_graph_and_add_to_level(graph_info input, level *new_level)
{
	graph_info extended;
	init_extended(input, &extended);
	
	add_edges(&extended, 0, (extended.n + WORDSIZE - 1) / WORDSIZE);
}

void test_extend_graph(void)
{
	graph_info g;
	int distances [25] = {
		0, 1, 1, 2, 2,
		1, 0, 2, 1, 3,
		1, 2, 0, 3, 1,
		2, 1, 3, 0, 4,
		2, 3, 1, 4, 0,
	};
	int m = (4 + WORDSIZE) / WORDSIZE;
	graph nauty_graph[m * 5];
	for (int i = 0; i < m * 5; i++)
		nauty_graph[i] = 0;
	ADDELEMENT(GRAPHROW(nauty_graph, 0, m), 1);
	ADDELEMENT(GRAPHROW(nauty_graph, 0, m), 2);
	ADDELEMENT(GRAPHROW(nauty_graph, 1, m), 0);
	ADDELEMENT(GRAPHROW(nauty_graph, 1, m), 3);
	ADDELEMENT(GRAPHROW(nauty_graph, 2, m), 0);
	ADDELEMENT(GRAPHROW(nauty_graph, 2, m), 4);
	ADDELEMENT(GRAPHROW(nauty_graph, 3, m), 1);
	ADDELEMENT(GRAPHROW(nauty_graph, 4, m), 2);
	
	g.distances = distances;
	g.nauty_graph = nauty_graph;
	g.n = 5;
	int g_k[5] = {2, 2, 2, 1 ,1};
	g.k = g_k;
	g.m = 4;
	g.max_k = 2;
	
	print_graph(g);
	
	extend_graph_and_add_to_level(g, NULL);
}
