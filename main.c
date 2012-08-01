#include "graph.h"
#include "hash_set.h"
#include "priority_queue.h"
#include <stdio.h>

int geng(int argc, char *argv[]); //entry point for geng

priority_queue *graph_queue;

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

bool graph_compare_gt(void *elem1, void *elem2)
{
	graph_info *graph1 = elem1, *graph2 = elem2;
	
	if(graph1->sum_of_distances > graph2->sum_of_distances)
		return true;
	else if(graph1->sum_of_distances < graph2->sum_of_distances)
		return false;
	return graph1->diameter > graph2->diameter;
}

void graph_delete(void *elem)
{
	graph_info *graph = elem;
	graph_info_destroy(graph);
}

int main(void)
{
	graph_queue = priority_queue_create(graph_compare_gt, graph_delete);
	call_geng(8, 4);
	while(priority_queue_num_elems(graph_queue))
	{
		graph_info *g = priority_queue_pull(graph_queue);
		print_graph(*g);
		graph_info_destroy(g);
	}
	return 0;
}

void geng_callback(FILE *file, graph *g, int n)
{
	graph_info *graph = graph_info_from_nauty(g, n);
	priority_queue_push(graph_queue, graph);
	if(priority_queue_num_elems(graph_queue) > P)
	{
		graph_info *g = priority_queue_pull(graph_queue);
		graph_info_destroy(g);
	}
}