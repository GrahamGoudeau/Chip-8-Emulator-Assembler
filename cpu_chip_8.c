#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>
#include "cpu_chip_8.h"

#define MEMORY_SIZE 0x1000
#define NUM_REGISTERS 0x10
#define STACK_SIZE 0x10
#define DIGIT_SPRITE_LEN 5

#define MEMORY_INIT_ERR "ERR - Fatal error during memory initialization: "
#define OPCODE_DECODE_ERR "ERR - Fatal error during opcode decoding: "
#define RUNTIME_ERR "ERR - Fatal error during run time: "

// first valid address of program instructions
const address PROG_START = 0x200;
const int default_window_height = 32;
const int default_window_width = 64;

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
    bool halt;

    pthread_mutex_t delay_mutex;
    pthread_mutex_t sound_mutex;
    pthread_t delay_decrement_thread;
    pthread_t sound_decrement_thread;

    WINDOW *chip_8_screen;
};

void *delay_thread(void *arg) {
    chip_8_cpu cpu = arg;
    while (true) {
        if (cpu->halt) {
            return NULL;
        }
        if (cpu->delay_timer) {
            while (cpu->delay_timer) {
                pthread_mutex_lock(&(cpu->delay_mutex));
                cpu->delay_timer--;
                pthread_mutex_unlock(&(cpu->delay_mutex));

                // sleep for 0.016 seconds = 1/60 seconds = 60 hz
                usleep(16666);
            }
        }
    }
    return NULL;
}

void *sound_thread(void *arg) {
    chip_8_cpu cpu = arg;
    while (true) {
        if (cpu->halt) {
            return NULL;
        }
    }
    return NULL;
}

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
    cpu->halt = false;
    pthread_mutex_init(&(cpu->delay_mutex), NULL);
    pthread_mutex_init(&(cpu->sound_mutex), NULL);

    // initialize random byte stream for RAND opcode
    srand(time(NULL));
    return cpu;
}

void free_cpu(chip_8_cpu cpu) {
    if (cpu) {
        pthread_mutex_destroy(&(cpu->delay_mutex));
        pthread_mutex_destroy(&(cpu->sound_mutex));
        free(cpu);
    }
}

void shutdown_cpu(chip_8_cpu cpu, int error_code) {
    free_cpu(cpu);
    endwin();
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

void store_digit_sprite(uint8_t sprite_arr[], address start_loc, chip_8_cpu cpu) {
    uint8_t i;
    for (i = 0; i < SPRITE_LEN; i++) {
        cpu->memory[start_loc + i] = sprite_arr[i];
    }
}

void store_digit_sprites(chip_8_cpu cpu) {
    uint8_t zero[5] = {0xF0,
                       0x90,
                       0x90,
                       0x90,
                       0xF0};
    store_digit_sprite(zero, 0 * SPRITE_LEN, cpu);
    uint8_t one[5] = {0x20,
                      0x60,
                      0x20,
                      0x20,
                      0x70};
    store_digit_sprite(one, 1 * SPRITE_LEN, cpu);
    uint8_t two[5] = {0xF0,
                      0x10,
                      0xF0,
                      0x80,
                      0xF0};
    store_digit_sprite(two, 2 * SPRITE_LEN, cpu);
    uint8_t three[5] = {0xF0,
                        0x10,
                        0xF0,
                        0x10,
                        0xF0};
    store_digit_sprite(three, 3 * SPRITE_LEN, cpu);
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
    store_digit_sprites(cpu);
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
    fprintf(stderr, OPCODE_DECODE_ERR "'Not implemented: 0x%04X'\n", instr);
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
    fprintf(stderr, RUNTIME_ERR "'Unrecognized opcode: 0x%04X'\n", instr);
    shutdown_cpu(cpu, 1);
}

static void invalid_mem_access(opcode instr, chip_8_cpu cpu) {
    fprintf(stderr, RUNTIME_ERR "'Invalid memory access at: 0x%04X'\n", instr);
    shutdown_cpu(cpu, 1);
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
                stack_underflow(cpu);
            }
            cpu->stack_pointer = stack_pointer;
            cpu->program_counter = cpu->stack[stack_pointer];
            cpu->performed_jump = true;
            break;
        }
        case 0xFD:
            cpu->halt = true;
            return;
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

static inline void set_vf_if(bool predicate, chip_8_cpu cpu) {
    if (predicate) {
        cpu->registers[0xf] = 1;
    }
    else {
        cpu->registers[0xf] = 0;
    }
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
            set_vf_if((vx > 128) && (vy > 128), cpu);
            break;
        case 5:
            result = vx - vy;
            set_vf_if((vx > vy), cpu);
            break;
        case 6:
            result = vx >> 1;
            set_vf_if(((vx & 0x01) == 1), cpu);
            break;
        case 7:
            result = vy - vx;
            set_vf_if((vy > vx), cpu);
            break;
        case 0xE:
            result = vx << 1;
            set_vf_if(((vx & 0x80) == 0x80), cpu);
            break;
        default:
            not_implemented(cpu, instr);
    }

    cpu->registers[reg_num1] = result;
}

static void handle_9_opcode(opcode instr, chip_8_cpu cpu) {
    if (get_last_nibble(instr) == 0) {
        nibble reg_num1 = get_second_nibble(instr);
        nibble reg_num2 = get_third_nibble(instr);
        if (reg_num1 != reg_num2) {
            cpu->skip_opcode = true;
        }
    }
    else {
        not_implemented(cpu, instr);
    }
}

static void handle_A_opcode(opcode instr, chip_8_cpu cpu) {
    address addr = get_last_three_nibbles(instr);
    cpu->address_register = addr;
}

static void handle_B_opcode(opcode instr, chip_8_cpu cpu) {
    chip_8_register offset = cpu->registers[0x0];
    address addr = get_last_three_nibbles(instr);
    cpu->program_counter = addr + offset;
    cpu->performed_jump = true;
}

static void handle_C_opcode(opcode instr, chip_8_cpu cpu) {
    uint8_t last_byte = get_last_byte(instr);
    chip_8_register reg_num = get_second_nibble(instr);
    uint8_t rand_byte = rand();
    cpu->registers[reg_num] = (last_byte & rand_byte);
}

static void handle_D_opcode(opcode instr, chip_8_cpu cpu) {
    not_implemented(cpu, instr);
}

static void handle_E_opcode(opcode instr, chip_8_cpu cpu) {
    /*
    uint8_t last_byte = get_last_byte(instr);
    switch (last_byte) {
        case 0x9E:
    }
    */
    not_implemented(cpu, instr);
}

static void handle_F_opcode(opcode instr, chip_8_cpu cpu) {
    int8_t reg_num = get_second_nibble(instr);
    switch (get_last_byte(instr)) {
        case 0x07:
            cpu->registers[reg_num] = cpu->delay_timer;
            break;
        case 0x0A:
            not_implemented(cpu, instr);
            break;
        case 0x15:
            pthread_mutex_lock(&(cpu->delay_mutex));
            cpu->delay_timer = cpu->registers[reg_num];
            pthread_mutex_unlock(&(cpu->delay_mutex));
            break;
        case 0x18:
            cpu->sound_timer = cpu->registers[reg_num];
            break;
        case 0x1E:
            cpu->address_register = cpu->address_register + cpu->registers[reg_num];
            break;
        case 0x29:
            cpu->address_register = reg_num;
            break;
        case 0x33:
            not_implemented(cpu, instr);
        case 0x55: {
            int8_t register_index;
            address start_addr = cpu->address_register;

            for (register_index = 0; register_index < reg_num; register_index++) {
                if (start_addr + register_index >= MEMORY_SIZE) {
                    invalid_mem_access(start_addr + register_index, cpu);
                }
                cpu->memory[start_addr + register_index] = cpu->registers[register_index];
            }
            break;
        }
        case 0x65: {
            int8_t register_index;
            address start_addr = cpu->address_register;

            for (register_index = 0; register_index < reg_num; register_index++) {
                if (start_addr + register_index >= MEMORY_SIZE) {
                    invalid_mem_access(instr, cpu);
                }
                cpu->registers[register_index] = cpu->memory[start_addr + register_index];
            }
            break;
        }
        default:
            not_implemented(cpu, instr);
    }
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
        default:
            invalid_opcode(instr, cpu);
    }
}

static inline void print_debug_info(FILE *debug_log, opcode instr, chip_8_cpu cpu) {
    fprintf(debug_log, "Execution loop info -- before processing 0x%04X:\n", instr);
    fprintf(debug_log, "\tProgram counter: %d (0x%04X)\n", cpu->program_counter, cpu->program_counter);
    fprintf(debug_log, "\tStack pointer: %d\n", cpu->stack_pointer);
    fprintf(debug_log, "\tDelay timer: %d (0x%04X)\n", cpu->delay_timer, cpu->delay_timer);
    fprintf(debug_log, "\tAddress register: %d (0x%04X)\n", cpu->address_register, cpu->address_register);
    fprintf(debug_log, "\tRegister contents:\n");
    int i;
    for (i = 0; i < NUM_REGISTERS; i++) {
        fprintf(debug_log, "\t\tReg %d: %hu\n", i, cpu->registers[i]);
    }
    fprintf(debug_log, "\n--------------\n\n");
    fflush(debug_log);
}

static WINDOW *create_window(int height, int width) {
    int max_height;
    int max_width;
    getmaxyx(stdscr, max_height, max_width);
    int window_height = (max_height <= height) ? max_height : height;
    int window_width = (max_width <= width) ? max_width : width;
    int starty = (LINES - window_height) / 2;
    int startx = (COLS - window_width) / 2;

    WINDOW *window;
    window = newwin(window_height, window_width, starty, startx);
    box(window, 0, 0);

    return window;
}

static void refresh_window(WINDOW *window) {
    box(window, 0, 0);
    refresh();
    wrefresh(window);
}

static WINDOW *init_ncurses(int height, int width) {
    initscr();
    curs_set(0);
    cbreak();
    WINDOW *window = create_window(height, width);
    refresh_window(window);

    return window;
}

void execute_loop(chip_8_cpu cpu, FILE *debug_log) {
    if (pthread_create(&(cpu->delay_decrement_thread), NULL, delay_thread, cpu) != 0) {
        shutdown_cpu(cpu, 1);
    }
    if (pthread_create(&(cpu->sound_decrement_thread), NULL, sound_thread, cpu) != 0) {
        fprintf(stderr, "Failed to start the sound register thread, exiting...\n");
        shutdown_cpu(cpu, 1);
    }
    pthread_detach(cpu->delay_decrement_thread);
    pthread_detach(cpu->sound_decrement_thread);

    cpu->chip_8_screen = init_ncurses(default_window_height, default_window_width);
    refresh_window(cpu->chip_8_screen);

    while (1) {
        if (cpu->program_counter >= MEMORY_SIZE) {
            fprintf(stderr, "ERR - Fatal memory error: invalid access at memory cell: '%d'\n", cpu->program_counter);
            shutdown_cpu(cpu, 1);
        }
        if (cpu->halt) {
            break;
        }
        opcode instr = fetch_opcode(cpu);
        if (debug_log) {
            print_debug_info(debug_log, instr, cpu);
        }

        execute_opcode(instr, cpu);
        if (cpu->performed_jump) {
            cpu->performed_jump = false;
            continue;
        }

        if (cpu->skip_opcode) {
            cpu->program_counter = cpu->program_counter + 2;
            cpu->skip_opcode = false;
        }
        else {
            cpu->program_counter = cpu->program_counter + 1;
        }
    }

    // allow 0.1 seconds for the threads to clean up their memory
    //usleep(100000);
    delwin(cpu->chip_8_screen);
    endwin();
}
