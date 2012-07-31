CC=gcc
GENG_MAIN=geng
OBJECTS=main.o priority_queue.o hash_set.o graph.o geng.o
CFLAGS=-I. -I./nauty24r2 -std=c99 -g
GENG_OBJECTS=nauty24r2/gtools.o nauty24r2/nauty1.o nauty24r2/nautil1.o nauty24r2/naugraph1.o #objects required by geng

all: fun_with_graphs

nauty: nauty24r2/Makefile
	cd nauty24r2 && make

nauty24r2/Makefile: nauty24r2/configure
	cd nauty24r2 && ./configure

geng.o: nauty nauty24r2/geng.c
	$(CC) -c nauty24r2/geng.c -o geng.o -DMAXN=32 -DGENG_MAIN=$(GENG_MAIN) -DOUTPROC=geng_callback

graph.o main.o: graph.h

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

fun_with_graphs: $(OBJECTS) $(GENG_OBJECTS)
	$(CC) $(OBJECTS) $(GENG_OBJECTS) -o $@

clean:
	rm *.o
	rm fun_with_graphs
	cd nauty24r2 && make clean

.PHONY: all nauty clean
