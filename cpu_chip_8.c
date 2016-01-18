#include <stdlib.h>
#include <stdint.h>
#include "cpu_chip_8.h"

#define MEMORY_SIZE 0x1000
#define NUM_REGISTERS 0x10
#define STACK_SIZE 0x10

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
