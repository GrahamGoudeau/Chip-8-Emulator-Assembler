# Chip-8 Emulator and Assembler
This project represents an emulator and associated assembler for the Chip-8 instruction set.  The emulator is written in C, and is implemented in `cpu_chip_8.c`.  The assembler is written in Python (2.7.9) and is implemented in `py8_assembler.py`

## Compilation
Use the provided Makefile to compile the `chip_8` emulator binary.  `chip_8` relies on `ncurses` to draw the screen and receive keystrokes; the `ncurses` library can be installed with

    $ sudo apt-get install libncurses5-dev

## Assembler Usage
The grammar for the assembly language can be found in `grammar.txt`.  The assembler supports labels for jumps and calls, and comments (lines beginning with `#`).  An example usage is:

    $ python py8_assembler.py timer.chasm timer.ch8
    
The binary file `timer.ch8` will be created.

## Emulator Usage
Using this `timer.ch8` file that was just produced, the emulator can be run with:

    $ ./chip_8 timer.ch8
    
Correct behavior of this command is the emulator runs, producing no output, for roughly one second before halting normally (the actual runtime will be closer to 1.1 seconds, since we must wait for `pthread` memory to be cleaned up).

## Instruction Set Documentation
The documentation for the chip-8 instruction set comes mainly from: 
http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
