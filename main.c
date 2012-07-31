#include "graph.h"
#include "hash_set.h"
#include <stdio.h>

int geng(int argc, char *argv[]); //entry point for geng

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

int main(void)
{
	call_geng(10, 4);
	printf("%u\n", num_graphs);
	return 0;
}

void geng_callback(FILE *file, graph *g, int n)
{
	
}