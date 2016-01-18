#ifndef CPU_CHIP_8_H
#define CPU_CHIP_8_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef uint16_t opcode;
typedef uint8_t chip_8_register;
typedef uint16_t special_register;
typedef uint16_t address;

struct chip_8_cpu;
typedef struct chip_8_cpu * chip_8_cpu;

chip_8_cpu initialize_cpu(void);

void free_cpu(chip_8_cpu);

void initialize_memory(chip_8_cpu, FILE *);
#endif
