CC ?= gcc
AR ?= ar
PREFIX = /usr/local

FLAGS = -O3 -std=c99 -Wall -g -pedantic 
#FLAGS = -O0 -std=c99 -Wall -g -pedantic -D_FAKE_MALLOC

SRC = src/point.c src/bounds.c src/node.c src/quadtree.c 
#SRC = src/point.c src/bounds.c src/node.c src/quadtree.c src/fake_malloc.c

OBJ = $(SRC:.c=.o)

all: test

build/libquadtree.a: $(OBJ)
	mkdir -p build
	$(AR) rcs $@ $^

bin/test: test.o $(OBJ)
	mkdir -p bin
	$(CC) $^ -lm -o $@

bin/benchmark: benchmark.o $(OBJ)
	mkdir -p bin
	$(CC) $^ -lm -o $@

clean:
	rm -fr bin build *.o src/*.o

%.o: %.c
	$(CC) $< $(FLAGS) -c -o $@

test: bin/test
	./$<

benchmark: bin/benchmark
	./$<

.PHONY: test clean benchmark
