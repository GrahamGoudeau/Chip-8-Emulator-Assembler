LDFLAGS=-lncurses -lpthread
FLAGS=-Wall -Wextra -g -c
CC=gcc
EXEC_NAME=chip_8

all: ${EXEC_NAME}

cpu_chip_8.o: cpu_chip_8.h cpu_chip_8.c
		${CC} ${FLAGS} cpu_chip_8.c -o $@

main.o: main.c
		${CC} ${FLAGS} $^ -o $@

${EXEC_NAME}: cpu_chip_8.o main.o
		${CC} $^ -o $@ ${LDFLAGS}

clean:
		rm *.o ${EXEC_NAME}
