#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "cpu_chip_8.h"

#define MEMORY_SIZE 0x1000
#define NUM_REGISTERS 0x10
#define STACK_SIZE 0x10
#define DIGIT_SPRITE_LEN 5

#define MEMORY_INIT_ERR "ERR - Fatal error during memory initialization: "

int debug = 1;

// first valid address of program instructions
const address PROG_START = 0x200;

struct chip_8_cpu {
    address memory[MEMORY_SIZE];

    // V0 through VF, hexadecimal
    chip_8_register registers[NUM_REGISTERS];
    special_register address_register;

    chip_8_register delay_timer;
    chip_8_register sound_timer;

    special_register program_counter;

    uint8_t stack_pointer;
    address stack[STACK_SIZE];
};

chip_8_cpu initialize_cpu(void) {
    chip_8_cpu cpu = malloc(sizeof(struct chip_8_cpu));
    if (!cpu) {
        return NULL;
    }

    int i;
    for (i = 0; i < MEMORY_SIZE; i++) {
        cpu->memory[i] = 0;
    }
    for (i = 0; i < NUM_REGISTERS; i++) {
        cpu->registers[i] = 0;
    }
    cpu->address_register = 0;
    cpu->delay_timer = 0;
    cpu->sound_timer = 0;
    cpu->program_counter = PROG_START;
    cpu->stack_pointer = 0;
    for (i = 0; i < STACK_SIZE; i++) {
        cpu->stack[i] = 0;
    }
    return cpu;
}

void free_cpu(chip_8_cpu cpu) {
    if (cpu) {
        free(cpu);
    }
}

static int read_16_bits(FILE *program_file) {
    int lower_byte, higher_byte;
    higher_byte = fgetc(program_file);
    if (higher_byte == EOF) {
        return EOF;
    }
    lower_byte = fgetc(program_file);
    if (lower_byte == EOF) {
        fprintf(stderr, MEMORY_INIT_ERR "'Input file contains malformed opcodes; odd number of bytes read'\n");
        exit(1);
    }

    opcode instruction = 0;
    instruction = higher_byte;
    instruction = instruction << 8;
    instruction |= lower_byte;
    return instruction;
}

void initialize_memory(chip_8_cpu cpu, FILE *program_file) {
    int new_16_bits;
    address destination = PROG_START;
    new_16_bits = read_16_bits(program_file);
    fprintf(stderr, "first time: %X\n", new_16_bits);
    while (new_16_bits != EOF && destination < MEMORY_SIZE) {
        if (debug) fprintf(stderr, "Destination: %d\n", destination);
        if (debug) fprintf(stderr, "Instruction: %X\n", (opcode)new_16_bits);
        cpu->memory[destination] = (opcode)new_16_bits;
        destination++;
        new_16_bits = read_16_bits(program_file);
    }
    if (destination == MEMORY_SIZE) {
        fprintf(stderr, MEMORY_INIT_ERR "'Program size exceeds chip-8 memory capacity'\n");
        exit(1);
    }
}
