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
#define RUNTIME_ERR "ERR - Fatal error during run time: "

int debug = true;

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

void shutdown_cpu(chip_8_cpu cpu, int error_code) {
    free_cpu(cpu);
    exit(error_code);
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
        cpu->memory[destination] = (opcode)new_16_bits;
        destination++;
        new_16_bits = read_16_bits(program_file);
    }
    if (destination == MEMORY_SIZE) {
        fprintf(stderr, MEMORY_INIT_ERR "'Program size exceeds chip-8 memory capacity'\n");
        shutdown_cpu(cpu, 1);
    }
}

static opcode fetch_opcode(chip_8_cpu cpu) {
    return cpu->memory[cpu->program_counter];
}

static inline uint8_t get_last_byte(opcode instr) {
    return instr & 0x00FF;
}

static void clear_display(void) {
    fprintf(stderr, "CLEAR SCREEN NOT IMPLEMENTED\n");
}

static void not_implemented(chip_8_cpu cpu, opcode instr) {
    fprintf(stderr, OPCODE_DECODE_ERR "'Not implemented: %04X'\n", instr);
    shutdown_cpu(cpu, 1);
}

static void stack_overflow(chip_8_cpu cpu) {
    fprintf(stderr, RUNTIME_ERR "'Call stack overflow on opcode CALL'\n");
    shutdown_cpu(cpu, 1);
}

static void stack_underflow(chip_8_cpu cpu) {
    fprintf(stderr, RUNTIME_ERR "Call stack empty but got opcode RET\n");
    shutdown_cpu(cpu, 1);
}

static void invalid_opcode(opcode instr, chip_8_cpu cpu) {
    fprintf(stderr, RUNTIME_ERR "'Unrecognized opcode: %04X'\n", instr);
    shutdown_cpu(cpu, 1);
}

static void handle_0_opcode(opcode instr, chip_8_cpu cpu) {
    if (instr == 0x00) {
        fprintf(stderr, "Got opcode 0x0000, quitting...\n");
        shutdown_cpu(cpu, 0);
    }

    // 0nnn opcode not implemented
    switch (get_last_byte(instr)) {
        case 0xE0:
            clear_display();
            break;
        case 0xEE: {
            int8_t stack_pointer = cpu->stack_pointer - 1;
            if (stack_pointer == -1) {
                stack_underflow(cpu);
            }
            cpu->stack_pointer = stack_pointer;
            cpu->program_counter = cpu->stack[stack_pointer];
            cpu->performed_jump = true;
            break;
        }
        default:
            not_implemented(cpu, instr);
    }
}

static inline address get_last_three_nibbles(opcode instr) {
    return (instr & 0x0FFF);
}

static void handle_1_opcode(opcode instr, chip_8_cpu cpu) {
    cpu->performed_jump = true;
    cpu->program_counter = get_last_three_nibbles(instr);
}

static void handle_2_opcode(opcode instr, chip_8_cpu cpu) {
    if (cpu->stack_pointer == STACK_SIZE) {
        stack_overflow(cpu);
    }

    cpu->performed_jump = true;

    // jump back to the instruction AFTER the CALL opcode
    cpu->stack[cpu->stack_pointer] = cpu->program_counter + 1;
    cpu->stack_pointer = cpu->stack_pointer + 1;

    address new_program_counter = get_last_three_nibbles(instr);
    cpu->program_counter = new_program_counter;
}

static inline nibble get_second_nibble(opcode instr) {
    return (instr & 0x0F00) >> 8;
}

static inline nibble get_third_nibble(opcode instr) {
    return (instr & 0x00F0) >> 4;
}

static inline nibble get_last_nibble(opcode instr) {
    return (instr & 0x000F);
}

static void handle_3_opcode(opcode instr, chip_8_cpu cpu) {
    uint8_t last_byte = get_last_byte(instr);
    nibble reg_number = get_second_nibble(instr);
    if (cpu->registers[reg_number] == last_byte) {
        cpu->skip_opcode = true;
    }
}

static void handle_4_opcode(opcode instr, chip_8_cpu cpu) {
    uint8_t last_byte = get_last_byte(instr);
    nibble reg_number = get_second_nibble(instr);
    if (cpu->registers[reg_number] != last_byte) {
        cpu->skip_opcode = true;
    }
}

static void handle_5_opcode(opcode instr, chip_8_cpu cpu) {
    if (get_last_nibble(instr) == 0) {
        nibble reg_number_1 = get_second_nibble(instr);
        nibble reg_number_2 = get_third_nibble(instr);
        if (cpu->registers[reg_number_1] == cpu->registers[reg_number_2]) {
            cpu->skip_opcode = true;
        }
    }
    else {
        invalid_opcode(instr, cpu);
    }
}

static void handle_6_opcode(opcode instr, chip_8_cpu cpu) {
    nibble reg_number = get_second_nibble(instr);
    uint8_t last_byte = get_last_byte(instr);
    cpu->registers[reg_number] = last_byte;
}

static void handle_7_opcode(opcode instr, chip_8_cpu cpu) {
    nibble reg_number = get_second_nibble(instr);
    uint8_t last_byte = get_last_byte(instr);
    cpu->registers[reg_number] += last_byte;
}

static void handle_8_opcode(opcode instr, chip_8_cpu cpu) {
    nibble indicator = get_last_nibble(instr);
    nibble reg_num1 = get_second_nibble(instr);
    nibble reg_num2 = get_third_nibble(instr);
    chip_8_register result;
    chip_8_register vx = cpu->registers[reg_num1];
    chip_8_register vy = cpu->registers[reg_num2];

    switch (indicator) {
        case 0:
            result = vy;
            break;
        case 1:
            result = vx | vy;
            break;
        case 2:
            result = vx & vy;
            break;
        case 3:
            result = vx ^ vy;
            break;
        case 4:
            result = vx + vy;
            if ((int)vx + (int)vy > 255) {
                cpu->registers[0xf] = 1;
            }
            else {
                cpu->registers[0xf] = 0;
            }
            break;
        case 5:
            result = vx - vy;
            if (vx > vy) {
                cpu->registers[0xf] = 1;
            }
            else {
                cpu->registers[0xf] = 0;
            }
            break;
        case 6:
            if ((vx & 0x01) == 1) {
                cpu->registers[0xf] = 1;
            }
            else {
                cpu->registers[0xf] = 0;
            }
            result = vx >> 1;
            break;

        case 7:
            if (vy > vx) {
                cpu->registers[0xf] = 1;
            }
            else {
                cpu->registers[0xf] = 0;
            }
            result = vy - vx;
            break;
        case 0xE:
            if ((vx & 0x80) == 0x80) {
                cpu->registers[0xf] = 1;
            }
            else {
                cpu->registers[0xf] = 0;
            }
            result = vx << 1;
            break;
        default:
            not_implemented(cpu, instr);
    }

    cpu->registers[reg_num1] = result;
}

static inline nibble get_first_nibble(opcode instr) {
    return (instr & 0xF000) >> 12;
}

static void execute_opcode(opcode instr, chip_8_cpu cpu) {
    nibble first_nibble = get_first_nibble(instr);
    switch (first_nibble) {
        case 0x0:
            return handle_0_opcode(instr, cpu);
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
            /*
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
        default:
            invalid_opcode(instr, cpu);
    }
}

void execute_loop(chip_8_cpu cpu) {
    while (1) {
        if (cpu->program_counter >= MEMORY_SIZE) {
            fprintf(stderr, "ERR - Fatal memory error: invalid access at memory cell: '%d'\n", cpu->program_counter);
            shutdown_cpu(cpu, 1);
        }
        opcode instr = fetch_opcode(cpu);
        if (debug) {
            fprintf(stderr, "Execution loop info -- before processing %04X:\n", instr);
            fprintf(stderr, "\tProgram counter: %d (%02X)\n", cpu->program_counter, cpu->program_counter);
            fprintf(stderr, "\tStack pointer: %d\n", cpu->stack_pointer);
            fprintf(stderr, "\tRegister contents:\n");
            int i;
            for (i = 0; i < NUM_REGISTERS; i++) {
                fprintf(stderr, "\t\tReg %d: %hu\n", i, cpu->registers[i]);
            }
            fprintf(stderr, "\n--------------\n\n");
        }

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
