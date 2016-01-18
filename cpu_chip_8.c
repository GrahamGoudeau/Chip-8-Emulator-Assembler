#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "cpu_chip_8.h"

#define MEMORY_SIZE 0x1000
#define NUM_REGISTERS 0x10
#define STACK_SIZE 0x10
#define DIGIT_SPRITE_LEN 5

#define MEMORY_INIT_ERR "ERR - Fatal error during memory initialization: "
#define OPCODE_DECODE_ERR "ERR - Fatal error during opcode decoding: "

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

    int8_t stack_pointer;
    address stack[STACK_SIZE];

    bool performed_jump;
    bool skip_opcode;
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
    cpu->performed_jump = false;
    cpu->skip_opcode = false;
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

static opcode fetch_opcode(chip_8_cpu cpu) {
    return cpu->memory[cpu->program_counter];
}

static inline uint8_t get_last_byte(opcode instr) {
    return instr & 0x000F;
}

static void clear_display(void) {
    fprintf(stderr, "CLEAR SCREEN NOT IMPLEMENTED\n");
}

static void handle_0_opcode(opcode instr, chip_8_cpu cpu) {
    // 0nnn opcode not implemented
    switch (get_last_byte(instr)) {
        case 0xE0:
            clear_display();
            break;
        case 0xEE: {
            int8_t stack_pointer = cpu->stack_pointer - 1;
            if (stack_pointer == -1) {
                fprintf(stderr, OPCODE_DECODE_ERR "Call stack empty but got opcode RET\n");
                exit(1);
            }
            cpu->stack_pointer = stack_pointer;
            cpu->program_counter = cpu->stack[stack_pointer];
            cpu->performed_jump = true;
        }
    }
}

static inline address get_last_three_nibbles(opcode instr) {
    return (instr & 0x0FFF);
}

static void handle_1_opcode(opcode instr, chip_8_cpu cpu) {
    cpu->performed_jump = true;
    cpu->program_counter = get_last_three_nibbles(instr);
}

static void handle_2_opcode(opcode instr, chip_8_cpu) {
    cpu->performed_jump = true;
    cpu->stack[cpu->stack_pointer] = cpu->program_counter;
    cpu->stack_pointer = cpu->stack_pointer + 1;

    address new_program_counter = get_last_three_nibbles(instr);
    cpu->program_counter = new_program_counter;
}

static inline nibble get_first_nibble(opcode instr) {
    return (instr & 0xF000) >> 6;
}

static void execute_opcode(opcode instr, chip_8_cpu cpu) {
    nibble first_nibble = get_first_nibble(instr);
    if (debug) fprintf(stderr, "Got nibble: %X\n", first_nibble);
    switch (first_nibble) {
        case 0x0:
            return handle_0_opcode(instr, cpu);
            /*
        case 0x1:
            return handle_1_opcode(instr, cpu);
        case 0x2:
            return handle_2_opcode(instr, cpu);
        case 0x3:
            return handle_3_opcode(instr, cpu);
        case 0x4:
            return handle_4_opcode(instr, cpu);
        case 0x5:
            return handle_5_opcode(instr, cpu);
        case 0x6:
            return handle_6_opcode(instr, cpu);
        case 0x7:
            return handle_7_opcode(instr, cpu);
        case 0x8:
            return handle_8_opcode(instr, cpu);
        case 0x9:
            return handle_9_opcode(instr, cpu);
        case 0xA:
            return handle_A_opcode(instr, cpu);
        case 0xB:
            return handle_B_opcode(instr, cpu);
        case 0xC:
            return handle_C_opcode(instr, cpu);
        case 0xD:
            return handle_D_opcode(instr, cpu);
        case 0xE:
            return handle_E_opcode(instr, cpu);
        case 0xF:
            return handle_F_opcode(instr, cpu);
            */
    }
}

void execute_loop(chip_8_cpu cpu) {
    while (1) {
        opcode instr = fetch_opcode(cpu);
        execute_opcode(instr, cpu);
        if (cpu->performed_jump) {
            // reset jumped flag
            cpu->performed_jump = false;
            continue;
        }

        if (cpu->skip_opcode) {
            cpu->program_counter = cpu->program_counter + 2;
            // reset skip flag
            cpu->skip_opcode = false;
        }
        else {
            cpu->program_counter = cpu->program_counter + 1;
        }
    }
}
