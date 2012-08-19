#include "level.h"
#include "naututil.h"
#include <string.h>
#include <mpi.h>
#include <signal.h>

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
		ret->sets[i] = hash_set_create(P * 1024, nauty_hash, nauty_compare,
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

bool add_graph_to_level(graph_info *new_graph, graph *gcan, level *my_level)
{
	unsigned i = new_graph->m - my_level->min_m;
	
	if(priority_queue_num_elems(my_level->queues[i]) >= my_level->p &&
	   graph_compare_gt(new_graph,
						   priority_queue_peek(my_level->queues[i])))
		return false;
	
	nauty_graph_with_n *set_entry = malloc(sizeof(nauty_graph_with_n));
	set_entry->n = new_graph->n;
	set_entry->g = gcan;
	if(!hash_set_add(my_level->sets[i], set_entry))
	{
		//this graph already exists
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

static void calc_canon(graph_info *g, graph *gcan)
{
	int m = (g->n + WORDSIZE - 1) / WORDSIZE;
	
	DEFAULTOPTIONS_GRAPH(options);
	statsblk stats;
	setword workspace[m * 50];
	int lab[g->n], ptn[g->n], orbits[g->n];
	
	options.getcanon = true;
	
	nauty(g->nauty_graph, lab, ptn, NULL, orbits,
		  &options, &stats, workspace, 50 * m, m, g->n, gcan);
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

void add_edges(graph_info *g, unsigned start, int extended_m, int rank, int n)
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
				
				add_edges(g, i + 1, extended_m, rank, n);
				
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
		graph *gcan = malloc(g->n * ((g->n + WORDSIZE - 1) / WORDSIZE) * sizeof(setword));
		calc_canon(temporary, gcan);
		send_graph(SLAVE_OUTPUT, 0, temporary);
		MPI_Send(gcan,
				 g->n * ((g->n + WORDSIZE - 1) / WORDSIZE),
				 MPI_SETWORD,
				 0,
				 SLAVE_OUTPUT,
				 MPI_COMM_WORLD);
		free(gcan);
		graph_info_destroy(temporary);
	}
}

graph_info* receive_graph(int tag, int src, int n)
{
	MPI_Status status;
	graph_info* g = malloc(sizeof(graph_info));
	g->distances = malloc(n * n * sizeof(int));
	g->k = malloc(n * sizeof(int));
	g->nauty_graph = malloc(n * ((n + WORDSIZE - 1) / WORDSIZE) * sizeof(setword));
	int receive_info[5];
	MPI_Recv(g->distances,
			 n*n,
			 MPI_INT,
			 src,
			 tag, MPI_COMM_WORLD, &status);
	MPI_Recv(g->k,
			 n,
			 MPI_INT, src, tag, MPI_COMM_WORLD, &status);
	MPI_Recv(g->nauty_graph,
			 n * ((n + WORDSIZE - 1) / WORDSIZE),
			 MPI_SETWORD,
			 src,
			 tag,
			 MPI_COMM_WORLD,
			 &status);
	MPI_Recv(receive_info,
			 5, MPI_INT,
			 src, tag, MPI_COMM_WORLD, &status);
	
	g->n = receive_info[0];
	g->sum_of_distances = receive_info[1];	
	g->m = receive_info[2];	
	g->diameter = receive_info[3];	
	g->max_k = receive_info[4];
	
	if (g->n != n)
	{
		fprintf(stderr, "Error: expecting graph size %d, got %d\n", n, g->n);
		raise(SIGINT);
	}
	
	return g;
}

void send_graph(int tag, int dest, graph_info* g)
{
	int send_info[5];
	send_info[0] = g->n;
	send_info[1] = g->sum_of_distances;
	send_info[2] = g->m;
	send_info[3] = g->diameter;
	send_info[4] = g->max_k;
	MPI_Send(g->distances, g->n*g->n,
			 MPI_INT, dest, tag, MPI_COMM_WORLD);
	MPI_Send(g->k, g->n, MPI_INT, dest, tag, MPI_COMM_WORLD);
	MPI_Send(g->nauty_graph, g->n * ((g->n - 1 + WORDSIZE) / WORDSIZE),
			 MPI_SETWORD,
			 dest,
			 tag,
			 MPI_COMM_WORLD);
	MPI_Send(send_info, 5, MPI_INT, dest, tag, MPI_COMM_WORLD);
}