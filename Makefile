LDFLAGS=-lncurses
FLAGS=-Wall -Wextra -g -c
CC=gcc

all: chip_8

cpu_chip_8.o: cpu_chip_8.h cpu_chip_8.c
		${CC} ${FLAGS} cpu_chip_8.c -o $@

main.o: main.c
		${CC} ${FLAGS} $^ -o $@

chip_8: cpu_chip_8.o main.o
		${CC} $^ -o $@ ${LDFLAGS}
