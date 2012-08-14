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
	//make sure max_k is sane
	if(max_k < 2 || max_k > (n - 1))
		return NULL;
	level *ret = malloc(sizeof(level));
	ret->n = n;
	ret->p = p;
	ret->max_k = max_k;
	ret->min_m = n - 1;
	ret->num_m = (n * max_k / 2) - ret->min_m + 1;
	
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
	printf("For n = %u\n", my_level->n);
	for(int i = 0; i < my_level->num_m; i++)
	{
		printf("m = %u:\n", i + my_level->min_m);
		while(priority_queue_num_elems(my_level->queues[i]))
		{
			graph_info *g = priority_queue_pull(my_level->queues[i]);
			print_graph(*g);
			graph_info_destroy(g);
		}
	}
}

bool add_graph_to_level(graph_info *new_graph, level *my_level)
{
	unsigned i = new_graph->m - my_level->min_m;
	
	if(priority_queue_num_elems(my_level->queues[i]) >= my_level->p &&
	   graph_compare_gt(new_graph,
						   priority_queue_peek(my_level->queues[i])))
		return false;
	
	int m = (new_graph->n + WORDSIZE - 1) / WORDSIZE;
	
	DEFAULTOPTIONS_GRAPH(options);
	statsblk stats;
	setword workspace[m * 50];
	int lab[new_graph->n], ptn[new_graph->n], orbits[new_graph->n];
	graph *gcan = malloc(new_graph->n * m * sizeof(setword));
	
	options.getcanon = true;
	
	nauty(new_graph->nauty_graph, lab, ptn, NULL, orbits,
		  &options, &stats, workspace, 50 * m, m, new_graph->n, gcan);
	
	nauty_graph_with_n *set_entry = malloc(sizeof(nauty_graph_with_n));
	set_entry->n = new_graph->n;
	set_entry->g = gcan;
	if(!hash_set_add(my_level->sets[i], set_entry))
	{
		//this graph already exists
		free(gcan);
		free(set_entry);
		return false;
	}
	
	_add_graph_to_level(new_graph, my_level);
	
	return true;
}

//Doesn't check/add to hash set
//Used for the first level created by geng
void _add_graph_to_level(graph_info *new_graph, level *my_level)
{
	unsigned i = new_graph->m - my_level->min_m;
	priority_queue_push(my_level->queues[i], new_graph);
	if(priority_queue_num_elems(my_level->queues[i]) > my_level->p)
	{
		graph_info *g = priority_queue_pull(my_level->queues[i]);
		graph_info_destroy(g);
	}
}

void init_extended(graph_info input, graph_info *extended)
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






