#include "level.h"
#include <stdio.h>
#include <mpi.h>
#include <assert.h>

#define NEW_LEVEL 0
#define SLAVE_KILL 1
#define SLAVE_INPUT 2
#define SLAVE_OUTPUT 3
#define SLAVE_REQUEST 4

#ifdef SETWORD_SHORT
#define MPI_SETWORD MPI_UNSIGNED_SHORT
#else 
#ifdef SETWORD_INT
#define MPI_SETWORD MPI_UNSIGNED
#else
#ifdef SETWORD_LONG
#define MPI_SETWORD MPI_UNSIGNED_LONG
#else
#define MPI_SETWORD MPI_UNSIGNED_LONG_LONG
#endif
#endif
#endif

int geng(int argc, char *argv[]); //entry point for geng

level *cur_level;

//Wrapper around the geng entry function
//n is the number of vertices
int call_geng(unsigned n, unsigned k)
{
	char n_buf[10], k_buf[10];
	char *geng_args[] = {
		"geng",
		"-ucq",
		"",
		""
	};
	sprintf(k_buf, "-D%d", k);
	sprintf(n_buf, "%d", n);
	geng_args[2] = k_buf;
	geng_args[3] = n_buf;
	return geng(4, geng_args);
}

//Each value of m is limited to P members;
//therefore, all graphs will be enumerated iff
//the value of m with the maximum number of graphs is <= P.
//So, to find the n we want to start at, we have to find the
//smallest n where the largest number of of graphs for any given
//m is smaller than P. The table below gives these values for
//each n, and was created using geng, using the command
//geng -ucv -D3 n
//where 3 is the max degree, and n is the number of nodes.
//These values need to be regenerated if max-k changes. (k = 3)

unsigned graph_sizes[] = {
	1, //n = 0
	1, //n = 1
	1, //n = 2, m = 1
	1, //n = 3, m = 2 or 3
	2, //n = 4, m = 3 or 4
	4, //n = 5, m = 5
	9, //n = 6, m = 6 or 7
	22, //n = 7, m = 8
	63, //n = 8, m = 9
	166, //n = 9, m = 11
	551, //n = 10, m = 12
	1694, //n = 11, m = 13
	5741, //n = 12, m = 15
	20818, //n = 13, m = 16
	74116, //n = 14, m = 17
	289254, //n = 15, m = 19
	1155398, //n = 16, m = 20
};

static graph_info* receive_graph(int tag, int src, int n, bool canon)
{
	MPI_Status status;
	graph_info* g = malloc(sizeof(graph_info));
	g->distances = malloc(n * n * sizeof(int));
	g->k = malloc(n * sizeof(int));
	g->nauty_graph = malloc(n * ((n + WORDSIZE - 1) / WORDSIZE) * sizeof(setword));
	if(canon)
		g->gcan = malloc(n * ((n + WORDSIZE - 1) / WORDSIZE) * sizeof(setword));
	else
		g->gcan = NULL;

	MPI_Recv(g->distances,
			 n*n,
			 MPI_INT,
			 src,
			 tag, MPI_COMM_WORLD, &status);
	MPI_Recv(g->nauty_graph,
			 n * ((n + WORDSIZE - 1) / WORDSIZE),
			 MPI_SETWORD,
			 src,
			 tag,
			 MPI_COMM_WORLD,
			 &status);
	if(canon)
		MPI_Recv(g->gcan,
				 n * ((n + WORDSIZE - 1) / WORDSIZE),
				 MPI_SETWORD,
				 src,
				 tag,
				 MPI_COMM_WORLD,
				 &status);

	g->n = n;
	calc_k(*g);
	g->sum_of_distances = calc_sum(*g);	
	g->m = calc_m(*g);	
	g->diameter = calc_diameter(*g);	
	g->max_k = calc_max_k(*g);

	assert(g->n == n);

	return g;
}

static void send_graph(int tag, int dest, graph_info* g, bool canon)
{
	MPI_Send(g->distances, g->n*g->n,
			 MPI_INT, dest, tag, MPI_COMM_WORLD);
	MPI_Send(g->nauty_graph, g->n * ((g->n - 1 + WORDSIZE) / WORDSIZE),
			 MPI_SETWORD,
			 dest,
			 tag,
			 MPI_COMM_WORLD);
	if(canon)
			MPI_Send(g->gcan, g->n * ((g->n - 1 + WORDSIZE) / WORDSIZE),
			 MPI_SETWORD,
			 dest,
			 tag,
			 MPI_COMM_WORLD);
}

bool slaves_done(bool *slave_done, int num_slaves)
{
	int i;
	for(i = 0; i < num_slaves; i++)
		if(!slave_done[i])
			return false;
	return true;
}

static void master(int size)
{
	MPI_Status status;
	//find n for geng
	unsigned n = 3;
	while(graph_sizes[n] <= P)
		n++;
	bool slave_done[size - 1];
	
	//setup cur_level for geng_callback()
	cur_level = level_create(n, P, MAX_K);
	
	call_geng(n, MAX_K);
	
	//Main loop
	while(n < 20)
	{
		printf("n = %u\n", n);
		level *new_level = level_create(n + 1, P, MAX_K);
		
		int i, j;
		for(i = 1; i < size; i++)
			for(j = 0; j < cur_level->num_m; j++)
				if(priority_queue_num_elems(cur_level->queues[j]))
				{
					graph_info *g = priority_queue_pull(cur_level->queues[j]);
					send_graph(SLAVE_INPUT, i, g, false);
					graph_info_destroy(g);
					break;
				}
		
		while(!level_empty(cur_level))
		{
			MPI_Recv(0, 0, MPI_INT, MPI_ANY_SOURCE, SLAVE_REQUEST, MPI_COMM_WORLD, &status);
			int i;
			for(i = 0; i < cur_level->num_m; i++)
				if(priority_queue_num_elems(cur_level->queues[i]))
				{
					graph_info *g = priority_queue_pull(cur_level->queues[i]);
					send_graph(SLAVE_INPUT, status.MPI_SOURCE, g, false);
					graph_info_destroy(g);
					break;
				}
		}
		
		n++;
		
		for(i = 0; i < size - 1; i++)
			slave_done[i] = false;
		
		while(!slaves_done(slave_done, size - 1))
		{
			MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			switch(status.MPI_TAG)
			{
				case SLAVE_REQUEST:
					MPI_Recv(0, 0, MPI_INT, status.MPI_SOURCE, SLAVE_REQUEST, MPI_COMM_WORLD, &status);
					MPI_Send(&n, 1, MPI_INT, status.MPI_SOURCE, NEW_LEVEL, MPI_COMM_WORLD);
					break;
				case SLAVE_OUTPUT:
				{
					int i, j;
					for(i = 0; i < new_level->num_m; i++)
					{
						int size;
						MPI_Recv(&size, 1,
								 MPI_INT,
								 status.MPI_SOURCE,
								 SLAVE_OUTPUT,
								 MPI_COMM_WORLD,
								 &status);
						for(j = 0; j < size; j++)
						{
							graph_info *g = receive_graph(SLAVE_OUTPUT, status.MPI_SOURCE, n, true);
							if(!add_graph_to_level(g, new_level))
								graph_info_destroy(g);
						}
					}
					slave_done[status.MPI_SOURCE - 1] = true;
					break;
				}
			}
		}
		
		level_delete(cur_level);
		cur_level = new_level;
	}
	
	int i;
	for (i = 1; i < size; i++)
		MPI_Send(0, 0, MPI_INT, i, SLAVE_KILL, MPI_COMM_WORLD);
	
	graph_info *best_graphs[cur_level->num_m];
	
	for(int i = 0; i < cur_level->num_m; i++)
	{
		while(priority_queue_num_elems(cur_level->queues[i]) > 1)
		{
			graph_info *g = priority_queue_pull(cur_level->queues[i]);
			graph_info_destroy(g);
		}
		best_graphs[i] = priority_queue_peek(cur_level->queues[i]);
	}
	
	graph_info *best_graph = NULL;
	for(int i = 0; i < cur_level->num_m; i++)
	{
		if(best_graph == NULL ||
		   (best_graphs[i] != NULL &&
		   (best_graphs[i]->sum_of_distances < best_graph->sum_of_distances ||
		   (best_graphs[i]->sum_of_distances == best_graph->sum_of_distances &&
		   best_graphs[i]->diameter < best_graph->diameter))))
			best_graph = best_graphs[i];
	}
	
	print_graph(*best_graph);
	
	level_delete(cur_level);
}

static void slave(int rank)
{
	MPI_Status status;
	unsigned n = 3;
	while(graph_sizes[n] <= P)
		n++;
	
	level *my_level = level_create(n + 1, P, MAX_K);
	
	while(true)
	{
		MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		switch(status.MPI_TAG)
		{
			case NEW_LEVEL:
				MPI_Recv(&n, 1, MPI_INT, 0, NEW_LEVEL, MPI_COMM_WORLD, &status);
				int i;
				for(i = 0; i < my_level->num_m; i++)
				{
					int num_elems = priority_queue_num_elems(my_level->queues[i]);
					MPI_Send(&num_elems,
							 1,
							 MPI_INT,
							 0,
							 SLAVE_OUTPUT,
							 MPI_COMM_WORLD);
					while(priority_queue_num_elems(my_level->queues[i]))
					{
						graph_info *g = priority_queue_pull(my_level->queues[i]);
						send_graph(SLAVE_OUTPUT, 0, g, true);
						graph_info_destroy(g);
					}
				}
				level_delete(my_level);
				my_level = level_create(n + 1, P, MAX_K);
				printf("n = %d (slave %d)\n", n, rank);
				break;
			case SLAVE_KILL:
				level_delete(my_level);
				return;
			case SLAVE_INPUT:
			{
				graph_info *g = receive_graph(SLAVE_INPUT, 0, n, false);
				extend_graph_and_add_to_level(*g, my_level);
				graph_info_destroy(g);
				
				MPI_Send(0, 0, MPI_INT, 0, SLAVE_REQUEST, MPI_COMM_WORLD);
				break;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);
	int rank, size = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	//MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);



	switch(rank)
	{
		case 0:
			master(size);
			break;
		default:
			slave(rank);
			break;
	}

	printf("I'm Done: %d/%d\n", rank, size);

	MPI_Finalize();
	return 0;
}

void geng_callback(FILE *file, graph *g, int n)
{
	graph_info *graph = graph_info_from_nauty(g, n);
	_add_graph_to_level(graph, cur_level);
}