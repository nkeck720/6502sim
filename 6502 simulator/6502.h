// 6502.h - for the 6502 CPU
//
#pragma once
#include <intsafe.h>

struct Flags {
	bool overflow;
	bool carry;
	bool decimal;
	bool zero;
	bool negative;
	bool interrupt;
	bool sc1;
	bool sc2;
};


static void ctrlcHandler(DWORD fdwCtrlType);

void start6502(unsigned int romStart, unsigned int romEnd, unsigned char *memory);
int run6502(unsigned int romStart, unsigned int romEnd, unsigned char *memory);
bool do6502Instruction(unsigned char inst, unsigned char op1, unsigned char op2, unsigned char *memory);
void waitCycles(unsigned char inst);
