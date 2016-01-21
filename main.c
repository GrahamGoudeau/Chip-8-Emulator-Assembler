#include <ncurses.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "cpu_chip_8.h"

#define required_input_ext "ch8"

static void print_usage(void) {
    fprintf(stderr, "Usage:\t./chip_8 [input.ch8]\n");
    fprintf(stderr, "\t(.ch8 file extension is required)\n");
}

// http://stackoverflow.com/questions/4849986/how-can-i-check-the-file-extensions-in-c
int ends_with(const char *name, const char *extension, size_t length) {
    const char *ldot = strrchr(name, '.');
    if (ldot != NULL)
    {
    if (length == 0)
        length = strlen(extension);
    return strncmp(ldot + 1, extension, length) == 0;

    }
    return 0;
}


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
    if (argc == 2 && (ends_with(argv[1], required_input_ext, 0) == 1)) {
        chip_8_cpu cpu = initialize_cpu();

        FILE *input = fopen(argv[1], "rb");
        if (!input) {
            fprintf(stderr, "Fatal error when opening input file: '%s'\n", argv[1]);
            exit(1);
        }
        initialize_memory(cpu, input);
        fclose(input);
        execute_loop(cpu);
        free_cpu(cpu);
    }
    else if (ends_with(argv[1], required_input_ext, 0) != 1) {
        fprintf(stderr, "Expected input file with extension '" required_input_ext "'\n");
        exit(1);
    }
    else {
        print_usage();
    }

    return EXIT_SUCCESS;
}
