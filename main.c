#include <ncurses.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "cpu_chip_8.h"

// sudo apt-get install libncurses5-dev
// http://stackoverflow.com/questions/4025891/create-a-function-to-check-for-key-press-in-unix-using-ncurses
// https://viget.com/extend/game-programming-in-c-with-the-ncurses-library
int main(int argc, char **argv) {
    /*
    initscr();
    noecho();
    curs_set(false);
    sleep(1);
    endwin();
    */
    if (argc > 1) {
        chip_8_cpu cpu = initialize_cpu();

        FILE *input = fopen(argv[1], "rb");
        initialize_memory(cpu, input);
        fclose(input);
        execute_loop(cpu);
        free_cpu(cpu);
    }
}
