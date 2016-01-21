#include <ncurses.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "cpu_chip_8.h"

#define required_input_ext "ch8"

static void print_usage(void) {
    fprintf(stderr, "Usage:\t./chip_8 [input.ch8] [-d (debug flag)]\n");
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
    int debug_flag = 0;
    char *input_filename = NULL;
    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "dp:")) != -1) {
        switch (c) {
            case 'd':
                debug_flag = 1;
                break;
            case 'p':
                input_filename = optarg;
                break;
            case '?':
                fprintf(stderr, "Unknown option: %c\n", optopt);
                print_usage();
                return 1;
            default:
                print_usage();
                return 1;
        }
    }

    if (optind < argc || input_filename == NULL || (ends_with(input_filename, required_input_ext, 0) != 1)) {
        print_usage();
        return 1;
    }

    FILE *input_file = fopen(input_filename, "rb");
    if (!input_file) {
        fprintf(stderr, "Fatal error when opening input file: '%s'\n", argv[1]);
        exit(1);
    }
    chip_8_cpu cpu = initialize_cpu();
    initialize_memory(cpu, input_file);
    fclose(input_file);
    execute_loop(cpu, debug_flag);
    free_cpu(cpu);

    return EXIT_SUCCESS;
}
