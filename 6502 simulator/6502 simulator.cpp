// 6502 simulator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "6502.h"


int main(int argc, char* argv[])
{
	char iDontCare;
	srand(time(NULL));
	/*
	 * Perform startup tasks.
	 * Start by checking args for ROM file
	 */
	if(argc == 1) {
		fprintf(stderr, "Error: no ROM specified\n");
		exit(EXIT_FAILURE);
	}
	FILE *romFile = fopen(argv[1], "rb");
	if(romFile == NULL) {
		fprintf(stderr, "Error: unable to open ROM %s", argv[1]);
		exit(EXIT_FAILURE);
	}
	unsigned int romStart, romLength, memorySize;
	printf("Bytes of memory? ");
	scanf("%i", &memorySize);
	printf("Starting location of ROM (decimal)? ");
	scanf("%i", &romStart);
	/* Allocate the array for memory size */
	fseek(romFile, 0L, SEEK_END);
	romLength = ftell(romFile);
	rewind(romFile);
	unsigned int romEnd = romStart + romLength;
	unsigned char *memoryMap = (unsigned char *)malloc(0x10000 * sizeof(unsigned char));
	if(!memoryMap) {
		printf("FATAL: Could not allocate enough memory for the virtual 6502 CPU.\n");
		exit(EXIT_FAILURE);
	}
	int i;
	printf("Creating memory map...\n");
	for(i = 0; i < 65535; i++) {
		/* Init the RAM to 0xFF */
		if(i <= memorySize)
			memoryMap[i] = 0xFF;
		else
			memoryMap[i] = rand();
	}
	printf("Loading ROM...\n");
	for(i = romStart; i < romEnd; i++) {
		unsigned char byteRead;
		fscanf(romFile, "%hhu", memoryMap[i]);
	}
	fclose(romFile);
	printf("ROM loaded.\n");
	/* Start the CPU, using the ROM values and memory map from before. */
	printf("SYS: Ready. Enter a single char to go.\nUSR: ");
	scanf("%c", &iDontCare);
	start6502(romStart, romEnd, memoryMap);
	int retVal = run6502(romStart, romEnd, memoryMap);
	/* When returned, give value of run6502's return to the system */
	printf("CPU thread exited with value %i.\n", retVal);
	free(memoryMap);
	exit(retVal);
}

