#include "level.h"
#include <stdio.h>

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



int main(void)
{
	printf("%d\n", MAXM);
	
	//find n for geng
	unsigned n = 3;
	while(graph_sizes[n] <= P)
		n++;
	
	//setup cur_level for geng_callback()
	cur_level = level_create(n, P, MAX_K);
	
	if(call_geng(n, MAX_K))
		return 1;
	
	//Main loop
	for(; n < 14; n++)
	{
		printf("n = %u\n", n);
		level *new_level = level_create(n + 1, P, MAX_K);
		level_extend(cur_level, new_level);
		level_delete(cur_level);
		cur_level = new_level;
	}
	
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
		   (best_graphs[i]->sum_of_distances == best_graph->sum_of_distances &&best_graphs[i]->diameter < best_graph->diameter))))
			best_graph = best_graphs[i];
	}
	
	print_graph(*best_graph);
	
	level_delete(cur_level);
	
	return 0;
}

void geng_callback(FILE *file, graph *g, int n)
{
	graph_info *graph = graph_info_from_nauty(g, n);
	_add_graph_to_level(graph, cur_level);
}