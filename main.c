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
	if(canon)
		MPI_Recv(g->gcan,
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

	assert(g->n == n);

	return g;
}

static void send_graph(int tag, int dest, graph_info* g, bool canon)
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
	if(canon)
			MPI_Send(g->gcan, g->n * ((g->n - 1 + WORDSIZE) / WORDSIZE),
			 MPI_SETWORD,
			 dest,
			 tag,
			 MPI_COMM_WORLD);
	MPI_Send(send_info, 5, MPI_INT, dest, tag, MPI_COMM_WORLD);
}

static bool slaves_done(bool *slave_done, int num_slaves)
{
	int i;
	for(i = 0; i < num_slaves; i++)
		if(!slave_done[i])
			return false;
	return true;
}

static void send_graphs(level *my_level, int dest)
{
	int i;
	for(i = 0; i < my_level->num_m; i++)
	{
		int num_elems = priority_queue_num_elems(my_level->queues[i]);
		MPI_Send(&num_elems,
				 1,
				 MPI_INT,
				 dest,
				 SLAVE_OUTPUT,
				 MPI_COMM_WORLD);
		while(priority_queue_num_elems(my_level->queues[i]))
		{
			graph_info *g = priority_queue_pull(my_level->queues[i]);
			send_graph(SLAVE_OUTPUT, dest, g, true);
			graph_info_destroy(g);
		}
	}
}

static void receive_graphs(level *new_level, int src, int n)
{
	MPI_Status status;
	int i, j;
	for(i = 0; i < new_level->num_m; i++)
	{
		int size;
		MPI_Recv(&size, 1,
				 MPI_INT,
				 src,
				 SLAVE_OUTPUT,
				 MPI_COMM_WORLD,
				 &status);
		for(j = 0; j < size; j++)
		{
			graph_info *g = receive_graph(SLAVE_OUTPUT, src, n, true);
			if(!add_graph_to_level(g, new_level))
				graph_info_destroy(g);
		}
	}
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
	while(n < 13)
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
					receive_graphs(new_level, status.MPI_SOURCE, n);
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

static void master_slave(int rank, int master, int slave_start, int num_slaves)
{
	bool requesting_slaves[num_slaves];
	bool slave_done[num_slaves];
	bool sending_request = false;
	MPI_Status status;
	unsigned n = 3;
	while(graph_sizes[n] <= P)
		n++;
	
	level *my_level = level_create(n + 1, P, MAX_K);
	
	int i;
	
	for(i = 0; i < num_slaves; i++)
		requesting_slaves[i] = true;
	
	while(true)
	{
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		switch(status.MPI_TAG)
		{
			//From master
			case SLAVE_KILL:
				for(i = 0; i < num_slaves; i++)
					MPI_Send(0, 0, MPI_INT, i + slave_start, SLAVE_KILL, MPI_COMM_WORLD);
				level_delete(my_level);
				return;
			
			//From master
			case SLAVE_INPUT:
				sending_request = false;
				graph_info *g = receive_graph(SLAVE_INPUT, master, n, false);
				for(i = 0; i < num_slaves; i++)
					if(requesting_slaves[i])
					{
						send_graph(SLAVE_INPUT, slave_start + i, g, false);
						requesting_slaves[i] = false;
						break;
					}
				for(i++; i < num_slaves; i++)
					if(requesting_slaves[i])
					{
						MPI_Send(0, 0, MPI_INT, master, SLAVE_REQUEST, MPI_COMM_WORLD);
						sending_request = true;
						break;
					}
				break;
			
			//From master
			case NEW_LEVEL:
				MPI_Recv(&n, 1, MPI_INT, status.MPI_SOURCE, NEW_LEVEL, MPI_COMM_WORLD, &status);
				
				//Any outstanding requests must be matched with NEW_LEVEL
				for(i = 0; i < num_slaves; i++)
					if(requesting_slaves[i])
						MPI_Send(&n, 1, MPI_INT, slave_start + i, NEW_LEVEL, MPI_COMM_WORLD);
				
				for(i = 0; i < num_slaves; i++)
					slave_done[i] = false;
				
				//Answer any new requests with NEW_LEVEL and collect all outputs
				while(!slaves_done(slave_done, num_slaves))
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
							receive_graphs(my_level, status.MPI_SOURCE, n);
							slave_done[status.MPI_SOURCE - slave_start] = true;
							break;
						}
					}
				}
				
				//Send collected graphs back to master
				send_graphs(my_level, master);
				level_delete(my_level);
				my_level = level_create(n + 1, P, MAX_K);
				
				//Reset requesting_slaves
				for(i = 0; i < num_slaves; i++)
					requesting_slaves[i] = true;
				
				printf("n = %d (master_slave %d)\n", n, rank);
				
				break;
			
			//From slave
			case SLAVE_REQUEST:
				MPI_Recv(0, 0, MPI_INT, status.MPI_SOURCE, SLAVE_REQUEST, MPI_COMM_WORLD, &status);
				requesting_slaves[status.MPI_SOURCE - slave_start] = true;
				if(!sending_request)
				{
					MPI_Send(0, 0, MPI_INT, master, SLAVE_REQUEST, MPI_COMM_WORLD);
					sending_request = true;
				}
				break;
				
				
			default:
				break;
		}
	}
}

static void slave(int rank, int master)
{
	MPI_Status status;
	unsigned n = 3;
	while(graph_sizes[n] <= P)
		n++;
	
	level *my_level = level_create(n + 1, P, MAX_K);
	
	while(true)
	{
		MPI_Probe(master, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		switch(status.MPI_TAG)
		{
			case NEW_LEVEL:
				MPI_Recv(&n, 1, MPI_INT, master, NEW_LEVEL, MPI_COMM_WORLD, &status);
				int i;
				send_graphs(my_level, master);
				level_delete(my_level);
				my_level = level_create(n + 1, P, MAX_K);
				printf("n = %d (slave %d)\n", n, rank);
				break;
			case SLAVE_KILL:
				level_delete(my_level);
				return;
			case SLAVE_INPUT:
			{
				graph_info *g = receive_graph(SLAVE_INPUT, master, n, false);
				extend_graph_and_add_to_level(*g, my_level);
				graph_info_destroy(g);
				
				MPI_Send(0, 0, MPI_INT, master, SLAVE_REQUEST, MPI_COMM_WORLD);
				break;
			}
		}
	}
}

#define NUM_LEVELS 3

//parent:child ratio for each level of the tree
int num_children[NUM_LEVELS - 1] = {2, 4};

static bool check_size(int size)
{
	int sum = 0, level, level_size = 1;
	for(level = 0; level < NUM_LEVELS; level++)
	{
		sum += level_size;
		if(level < NUM_LEVELS - 1)
			level_size *= num_children[level];
	}
	
	return sum == size;
}

static void get_position(int rank, int *slave_start, int *num_slaves, int *master)
{
	int level_start = 0, level_size = 1, level = 0;
	while(rank >= level_start + level_size)
	{
		level_start += level_size;
		level_size *= num_children[level];
		level++;
		assert(level < NUM_LEVELS);
	}
	
	int level_pos = rank - level_start;
	
	if(level < NUM_LEVELS - 1)
	{
		*slave_start = level_start + level_size;
		*slave_start += level_pos * num_children[level];
		*num_slaves = num_children[level];
	}
	else
		*slave_start = -1;
	
	int old_level_size = level_size / num_children[level - 1];
	int master_pos = level_pos / num_children[level - 1];
	int old_level_start = level_start - old_level_size;
	*master = old_level_start + master_pos;
}

int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);
	int rank, size = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

	assert(check_size(size));

	if(rank == 0)
		master(num_children[0] + 1);
	else
	{
		int slave_start, num_slaves, master;
		get_position(rank, &slave_start, &num_slaves, &master);
		if(slave_start == -1)
			slave(rank, master);
		else
			master_slave(rank, master, slave_start, num_slaves);
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