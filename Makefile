LDFLAGS=-lncurses

chip_8:
		gcc *.c -o chip_8 ${LDFLAGS}

all: chip_8
