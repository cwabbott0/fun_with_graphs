CC=mpicc
GENG_MAIN=geng
OBJECTS=main.o priority_queue.o hash_set.o graph.o level.o geng.o
CFLAGS=-I. -I./nauty24r2 -std=c99 -g
NAUTY_OBJECTS=nauty24r2/gtools.o nauty24r2/nauty.o nauty24r2/nautil.o nauty24r2/naugraph.o nauty24r2/naututil.o nauty24r2/rng.o

all: fun_with_graphs

nauty: nauty24r2/makefile
	cd nauty24r2 && make

nauty24r2/makefile: nauty24r2/configure
	cd nauty24r2 && ./configure

geng.o: nauty nauty24r2/geng.c
	$(CC) -c nauty24r2/geng.c -o geng.o -DMAXN=32 -DGENG_MAIN=$(GENG_MAIN) -DOUTPROC=geng_callback

graph.o main.o level.o: graph.h
level.o main.o: level.h

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

fun_with_graphs: $(OBJECTS) $(NAUTY_OBJECTS)
	$(CC) $(OBJECTS) $(NAUTY_OBJECTS) -o $@

clean:
	rm *.o
	rm fun_with_graphs
	cd nauty24r2 && make clean

.PHONY: all nauty clean
