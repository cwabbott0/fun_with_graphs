#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <stdbool.h>
#include "level.h"




int geng(int argc, char *argv[]); //entry point for geng

level *cur_level;
//


//
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




bool worker_status(bool *busy, int size)
{
	int i = 0;
	for (; i < size; i++)
	{	
		if (busy[i] == true)
			return true;
	}
	return false;
}







int master(int rank, int size)
{
	bool *busy = calloc((size-1)*sizeof(bool),1);
	MPI_Status status;
	unsigned n = 3;
	while(graph_sizes[n] <= P)
		n++;
	
	
	//setup cur_level for geng_callback()
	cur_level = level_create(n, P, MAX_K);
	
	if(call_geng(n, MAX_K))
		return 1;



	int *send_distances = malloc(sizeof(int)*n*n);
	int *receive_distances= malloc(sizeof(int)*(n + 1)*(n + 1));
	int *send_k = malloc(sizeof(int)*n);
	int *receive_k = malloc(sizeof(int)*(n + 1));
	graph *send_nauty = malloc(n * ((n -1 + WORDSIZE) / WORDSIZE) * sizeof(setword));
	graph *receive_nauty = malloc((n + 1) * ((n + WORDSIZE) / WORDSIZE) * sizeof(setword));
	int send_info[5] = {0};
	int receive_info[5] = {0};


	level *new_level = level_create(n + 1, P, MAX_K);
	int j = 1;

	for (; j < size; j++)
	{
		int i = 0;
		for(; i < cur_level->num_m; i++)
		{
			if(priority_queue_num_elems(cur_level->queues[i]))	
			{
				graph_info *g = priority_queue_pull(cur_level->queues[i]);
				send_distances = g->distances;
				send_k = g->k;
				send_nauty = g->nauty_graph;
				send_info[0] = g->n;
				send_info[1] = g->sum_of_distances;
				send_info[2] = g->m;
				send_info[3] = g->diameter;
				send_info[4] = g->max_k;
			
			busy[j - 1] = true;
			MPI_Send(send_distances, n*n, MPI_INT, j, SLAVE_INPUT, MPI_COMM_WORLD);
			MPI_Send(send_k, n, MPI_INT, j, SLAVE_INPUT, MPI_COMM_WORLD);
			MPI_Send(send_nauty, (n * ((n -1 + WORDSIZE) / WORDSIZE) * sizeof(setword)), MPI_INT, j, SLAVE_INPUT, MPI_COMM_WORLD);
			MPI_Send(send_info, 5, MPI_INT, j, SLAVE_INPUT, MPI_COMM_WORLD);
			break;	
			}
		}

	}

	int i = 0;
	while(n < 90)
	{
		
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);	
		switch(status.MPI_TAG)
		{
			case(SLAVE_OUTPUT):
				
				MPI_Recv(receive_distances, (n + 1)*(n + 1), MPI_INT, status.MPI_SOURCE, SLAVE_OUTPUT, MPI_COMM_WORLD, &status);
				MPI_Recv(receive_k, n + 1, MPI_INT, status.MPI_SOURCE, SLAVE_OUTPUT, MPI_COMM_WORLD, &status);
				MPI_Recv(receive_nauty, ((n + 1) * ((n + WORDSIZE) / WORDSIZE) * sizeof(setword)), MPI_INT, status.MPI_SOURCE, SLAVE_OUTPUT, MPI_COMM_WORLD, &status);
				MPI_Recv(receive_info, 5, MPI_INT, status.MPI_SOURCE, SLAVE_OUTPUT, MPI_COMM_WORLD, &status);
				//receive all else
				
				graph_info *temporary = malloc(sizeof(graph_info));
				temporary->distances = malloc(sizeof(int)*(n + 1)*(n + 1));
				temporary->k = malloc(sizeof(int)*(n + 1));
				//construct using recv
			
					temporary->distances = receive_distances;
					temporary->k = receive_k;
					temporary->nauty_graph = receive_nauty;
					temporary->n = receive_info[0];	
					temporary->sum_of_distances = receive_info[1];	
					temporary->m = receive_info[2];	
					temporary->diameter = receive_info[3];	
					temporary->max_k = receive_info[4];				

					

				if (!(add_graph_to_level(temporary, new_level)));
					graph_info_destroy(temporary);
		
		
				break;	
			case(SLAVE_REQUEST):
				busy[status.MPI_SOURCE - 1] = false;
				if (priority_queue_num_elems(cur_level->queues[cur_level->num_m - 1]))//Check if there are elements left to give out
					for(; i < cur_level->num_m; i++)
					{
						if(priority_queue_num_elems(cur_level->queues[i]))	
						{
							graph_info *g = priority_queue_pull(cur_level->queues[i]);
							send_distances = g->distances;
							send_k = g->k;
							send_nauty = g->nauty_graph;
							send_info[0] = g->n;
							send_info[1] = g->sum_of_distances;
							send_info[2] = g->m;
							send_info[3] = g->diameter;
							send_info[4] = g->max_k;
						busy[status.MPI_SOURCE - 1] = true;
						MPI_Send(send_distances, n*n, MPI_INT, status.MPI_SOURCE, SLAVE_INPUT, MPI_COMM_WORLD);//Send to source
						MPI_Send(send_k, n, MPI_INT, status.MPI_SOURCE, SLAVE_INPUT, MPI_COMM_WORLD);
						MPI_Send(send_nauty, (n * ((n -1 + WORDSIZE) / WORDSIZE) * sizeof(setword)), MPI_INT, status.MPI_SOURCE, SLAVE_INPUT, MPI_COMM_WORLD);
						MPI_Send(send_info, 5, MPI_INT, status.MPI_SOURCE, SLAVE_INPUT, MPI_COMM_WORLD);
						//Send all else
						
						break;	
						}
					}
				else	//None left in the level
				{	
					//New level
					n++;
					MPI_Send(&n, 1, MPI_INT, status.MPI_SOURCE, NEW_LEVEL, MPI_COMM_WORLD);//New level for the current proc
					i = 0;
					while(worker_status(busy, size))
					{
						
						MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);	
						switch(status.MPI_TAG)
						{
							case(SLAVE_OUTPUT):
								
								MPI_Recv(receive_distances, (n)*(n), MPI_INT, status.MPI_SOURCE, SLAVE_OUTPUT, MPI_COMM_WORLD, &status);
								MPI_Recv(receive_k, n, MPI_INT, status.MPI_SOURCE, SLAVE_OUTPUT, MPI_COMM_WORLD, &status);
								MPI_Recv(receive_nauty, ((n) * ((n - 1 + WORDSIZE) / WORDSIZE) * sizeof(setword)), MPI_INT, status.MPI_SOURCE, SLAVE_OUTPUT, MPI_COMM_WORLD, &status);
								MPI_Recv(receive_info, 5, MPI_INT, status.MPI_SOURCE, SLAVE_OUTPUT, MPI_COMM_WORLD, &status);
								//receive all else
								
								graph_info *temporary = malloc(sizeof(graph_info));
								temporary->distances = malloc(sizeof(int)*n*n);
								temporary->k = malloc(sizeof(int)*n);
								//construct using recv
									temporary->distances = receive_distances;
									temporary->k = receive_k;
									temporary->nauty_graph = receive_nauty;
									temporary->n = receive_info[0];	
									temporary->sum_of_distances = receive_info[1];	
									temporary->m = receive_info[2];	
									temporary->diameter = receive_info[3];	
									temporary->max_k = receive_info[4];				


								if (!(add_graph_to_level(temporary, new_level)));
									graph_info_destroy(temporary);
								
								break;

							case(SLAVE_REQUEST):
								busy[status.MPI_SOURCE - 1] = false;
								MPI_Send(&n, 1, MPI_INT, status.MPI_SOURCE, NEW_LEVEL, MPI_COMM_WORLD);
								break;
						}
					}

					send_distances = realloc(send_distances, sizeof(int)*n*n);	
					receive_distances = realloc(receive_distances, sizeof(int)*(n + 1)*(n + 1));//Update send/receive for master
					send_k = realloc(send_k, sizeof(int)*n);
					receive_k = realloc(receive_k, sizeof(int)*(n + 1));
					send_nauty = realloc(send_nauty, n * ((n + WORDSIZE) / WORDSIZE) * sizeof(setword));
					receive_nauty = realloc(receive_nauty, (n + 1) * ((n + 1 + WORDSIZE) / WORDSIZE) * sizeof(setword));
					

					level_delete(cur_level);
					cur_level = new_level;

					level *new_level = level_create(n, P, MAX_K);
		
					int j = 1;
					for (; j < size; j++)
					{
						int i = 0;
						for(; i < cur_level->num_m; i++)
						{
							if(priority_queue_num_elems(cur_level->queues[i]))	
							{
								graph_info *g = priority_queue_pull(cur_level->queues[i]);
								send_distances = g->distances;
								send_k = g->k;
								send_nauty = g->nauty_graph;
								send_info[0] = g->n;
								send_info[1] = g->sum_of_distances;
								send_info[2] = g->m;
								send_info[3] = g->diameter;
								send_info[4] = g->max_k;								
			
							busy[j - 1] = true;
							MPI_Send(send_distances, n*n, MPI_INT, j, SLAVE_INPUT, MPI_COMM_WORLD);
							MPI_Send(send_k, n, MPI_INT, j, SLAVE_INPUT, MPI_COMM_WORLD);
							MPI_Send(send_nauty, (n * ((n -1 + WORDSIZE) / WORDSIZE) * sizeof(setword)), MPI_INT, j, SLAVE_INPUT, MPI_COMM_WORLD);
							MPI_Send(send_info, 5, MPI_INT, j, SLAVE_INPUT, MPI_COMM_WORLD);
							break;		
							}
						}
				
					}

	
					



				}
		}
	}
	i = 1;
	for (; i < size; i++)
		MPI_Send(0, 0, MPI_INT, i, SLAVE_KILL, MPI_COMM_WORLD);	

	return 0;
}


int slave(int rank)
{
	
	MPI_Status status;
	int n = 10;	
	int receive_info[5] = {0};
	int *receive_k = malloc(sizeof(int)*n);
	int *receive_distances= malloc(sizeof(int)*n*n);
	graph *receive_nauty = malloc((n) * ((n - 1 + WORDSIZE) / WORDSIZE) * sizeof(setword));
	int i = 0;
	
	while(1)
	{
		MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		switch(status.MPI_TAG)
		{
			case NEW_LEVEL:
				MPI_Recv(&n, 1, MPI_INT, 0, NEW_LEVEL, MPI_COMM_WORLD, &status);
				receive_k = realloc(receive_k, sizeof(int)*n);
				receive_distances= realloc(receive_distances, sizeof(int)*n*n);
				receive_nauty = realloc(receive_nauty, (n) * ((n + WORDSIZE) / WORDSIZE) * sizeof(setword));
				break;
			case SLAVE_KILL:
				return 0;
				break;
			case SLAVE_INPUT:
			{
				
				graph_info *g = malloc(sizeof(graph_info));
				g->distances = malloc(sizeof(int)*n*n);
				g->k = malloc(sizeof(int)*n);
				MPI_Recv(receive_distances, n*n, MPI_INT, 0, SLAVE_INPUT, MPI_COMM_WORLD, &status);
				
				MPI_Recv(receive_k, n, MPI_INT, 0, SLAVE_INPUT, MPI_COMM_WORLD, &status);
				MPI_Recv(receive_nauty, ((n) * ((n - 1 + WORDSIZE) / WORDSIZE) * sizeof(setword)), MPI_INT, 0, SLAVE_INPUT, MPI_COMM_WORLD, &status);
				MPI_Recv(receive_info, 5, MPI_INT, 0, SLAVE_INPUT, MPI_COMM_WORLD, &status);

				g->distances = receive_distances;
				
				g->k = receive_k;
				g->nauty_graph = receive_nauty;
				g->n = receive_info[0];	
				g->sum_of_distances = receive_info[1];	
				g->m = receive_info[2];	
				g->diameter = receive_info[3];	
				g->max_k = receive_info[4];				
												
				//construct g

			
			
				//calculations		
				graph_info extended;
				
				init_extended(*g, &extended);
				
				add_edges(&extended, 0, (extended.n + WORDSIZE - 1) / WORDSIZE, rank, n);

				MPI_Send(0, 0, MPI_INT, 0, SLAVE_REQUEST, MPI_COMM_WORLD);	

				break;
			}
		}
	}	
	
	return 0;

}

int main(int argc, char* argv[])
{

MPI_Init(&argc, &argv);
int rank, size = 0;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
MPI_Comm_size(MPI_COMM_WORLD, &size);



switch(rank)
{
	case 0:
				
		master(rank, size);
		MPI_Send(0, 0, MPI_INT, 1, SLAVE_KILL, MPI_COMM_WORLD);	
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








