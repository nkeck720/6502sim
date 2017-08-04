// for the 6502 CPU
//

#include "stdafx.h"
#include "6502.h"
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <intsafe.h>
#include <limits.h>

#if CHAR_BIT == 8

static unsigned int programCounter;
static unsigned int stackPointer;
static unsigned int a, x, y;
static struct Flags flags;
static unsigned char currentInstruction, op1, op2;
static bool halted;
static const unsigned int stackPage = 0x100;
static bool servingBRK = false;
static bool servingIRQ = false;

static void ctrlcHandler(DWORD fdwCtrlType) {
	if(fdwCtrlType != CTRL_C_EVENT)
		return;
	fprintf(stderr, "SYS: caught ctrl-c, doing clean shutdown\n");
	halted = true;
}

/*
 * The number of operands:
 * -1 - invalid instruction
 * 0-3 = number of operands
 */

static const int numberOperands[0x100] = { 
	1, 1, -1, -1, -1, 1, 1, -1, 0, 1, 0, -1, -1, 2, 2, -1,
	1, 1, -1, -1, -1, 1, 1, -1, 0, 2, -1, -1, -1, 2, 2, -1,
	2, 1, -1, -1, 1, 1, 1, -1, 0, 1, 0, -1, 2, 2, 2, -1,
	1, 1, -1, -1, -1, 1, 1, -1, 0, 2, -1, -1, -1, 2, 2, -1,
	0, 1, -1, -1, -1, 1, 1, -1, 0, 1, 0, -1, 2, 2, 2, -1, 
	1, 1, -1, -1, -1, 1, 1, -1, 0, 2, -1, -1, -1, 2, 2, -1,
	0, 1, -1, -1, -1, 1, 1, -1, 0, 1, 0, -1, 2, 2, 2, -1,
	1, 1, -1, -1, -1, 1, 1, -1, 0, 2, -1, -1, -1, 2, 2, -1,
	-1, 1, -1, -1, 1, 1, 1, -1, 0, -1, 0, -1, 2, 2, 2, -1, 
	1, 1, -1, -1, 1, 1, 1, -1, 0, 1, 0, -1, -1, 2, -1, -1, 
	1, 1, 1, -1, 1, 1, 1, -1, 0, 1, 0, -1, 2, 2, 2, -1, 
	1, 1, -1, -1, 1, 1, 1, -1, 0, 1, 0, -1, 2, 2, 2, -1, 
	1, 1, -1, -1, 1, 1, 1, -1, 0, 1, 0, -1, 2, 2, 2, -1, 
	1, 1, -1, -1, -1, 1, 1, -1, 0, 2, -1, -1, -1, 2, 2, -1, 
	1, 1, -1, -1, 1, 1, 1, 1, 0, 1, 0, -1, 2, 2, 2, -1, 
	1, 1, -1, -1, -1, 1, 1, -1, 0, 2, -1, -1, -1, 2, 2, -1
};

void start6502(unsigned int romStart, unsigned int romEnd, unsigned char *memory) {
	/* Init the flags */
	stackPointer = 0xFD;
	programCounter = memory[(unsigned int)0xFFFD];
	programCounter = programCounter << 8;
	programCounter += memory[(unsigned int)0xFFFC];
	flags.interrupt = true;
	flags.sc1 = true;
	flags.sc2 = true;
	flags.decimal = false;
	flags.carry = rand() % 2;
	flags.negative = rand() % 2;
	flags.overflow = rand() % 2;
	flags.zero = rand() % 2;
	currentInstruction = memory[programCounter];
	halted = false;
}

int run6502(unsigned int romStart, unsigned int romEnd, unsigned char *memory) {
	/* Set up ctrl-c handler */
	if(SetConsoleCtrlHandler((PHANDLER_ROUTINE)ctrlcHandler, TRUE)) {
		fprintf(stderr, "SYS: Monitoring ctrl-c\n");
	}
	else
		fprintf(stderr, "SYS: Unable to set handler, ctrl-c may result in memory leak\n");
	/* In a loop, fetch instructions and operands */
	int i;
	while(halted == false) {
		/* Check stack */
		if(stackPointer <= 0x10)
			fprintf(stderr, "WARN: Less than 16 bytes on stack, may overflow soon\n");
		/* Get the next instruction */
		currentInstruction = memory[programCounter];
		/* Fetch the number of operands to get */
		if(numberOperands[currentInstruction] == -1) {
			fprintf(stderr, "WARN: Invalid instruction (%u) at %u, skipping\n", currentInstruction, programCounter);
			if(programCounter == 0xFFFF)
				programCounter = 0x0000;
			else
				programCounter++;
			continue;
		}
		for(i = numberOperands[currentInstruction]; i > 0; i++) {
			if(i == 2) {
				if(programCounter == 0xFFFF)
					programCounter = 0x0000;
				else
					programCounter++;
				op1 = memory[programCounter];
			}
			else if(i == 1) {
				if(programCounter == 0xFFFF)
					programCounter = 0x0000;
				else
					programCounter++;
				op2 = memory[programCounter];
			}
		}
		/* Interpret the instruction */
		halted = do6502Instruction(currentInstruction, op1, op2, memory);
		waitCycles(currentInstruction);
		/* Next instruction */
		if(programCounter == 0xFFFF)
			programCounter = 0x0000;
		else
			programCounter++;
		fprintf(stderr, "SYS: CPU halted\n");
	}
	return EXIT_SUCCESS;
}

bool do6502Instruction(unsigned char inst, unsigned char op1, unsigned char op2, unsigned char *memory) {
	/* Interpret the instructions, one by one. */
	unsigned int pushByte;
	unsigned int stackLocation;
	unsigned char statusByte = 0x00;
	if(inst == 0x00) {
		/* BRK, push PC low, high, and status */
		pushByte = programCounter & 0x00FF;
		stackLocation = stackPointer + stackPage;
		memory[stackLocation] = (unsigned char)pushByte;
		stackPointer--;
		stackLocation = stackPointer + stackPage;
		pushByte = programCounter & 0xFF00;
		pushByte >> 8;
		memory[stackLocation] = (unsigned char)pushByte;
		stackPointer--;
		stackLocation = stackPointer + stackPage;
		if(flags.negative == true)
			statusByte = statusByte | 0x80;
		if(flags.zero == true)
			statusByte = statusByte | 0x40;
		if(flags.carry == true)
			statusByte = statusByte | 0x08;
		if(flags.interrupt == true)
			statusByte = statusByte | 0x04;
		if(flags.decimal == true)
			statusByte = statusByte | 0x02;
		if(flags.overflow == true)
			statusByte = statusByte | 0x01;
		memory[stackLocation] = statusByte;
		stackPointer--;
		/* Get the vector and set the flags */
		programCounter = memory[0xFFFE];
		programCounter << 8;
		programCounter += memory[0xFFFF];
		servingBRK = true;
		flags.interrupt = true;
		return false;
	}
	else {
		fprintf(stderr, "ERR: invalid instruction given to do6502Instruction() (address %u)\nSYS: decimal opcode %u\n", (unsigned int)(programCounter - numberOperands[currentInstruction]), currentInstruction);
		return true;
	}
}

void waitCycles(unsigned char inst) {
	fprintf(stderr, "SYS: waitCycles() called, but not yet implemented\nSYS: opcode to wait for %u\n", currentInstruction);
}

#endif