#include <stdio.h>
#include <stdlib.h>
#define $sp 29
// Define R's
#define R 0
#define add 32
#define addu 33
#define sub 34
#define and 36
#define or 37
#define xor 38
#define nor 39
#define nand 40
#define slt 42
#define sll 0
#define srl 2
#define sra 3
#define jr 8
// Define others'
#define addi 8
#define addiu 9
#define lw 35
#define lh 33
#define lhu 37
#define lb 32
#define lbu 36
#define sw 43
#define sh 41
#define sb 40
#define lui 15
#define andi 12
#define ori 13
#define nori 14
#define slti 10
#define beq 4
#define bne 5
#define bgtz 7
#define j 2
#define jal 3
#define halt 63

unsigned iImgLen, dImgLen, iImgLenResult, dImgLenResult;
unsigned reg[32], PC, insPos;
char *iImgBuffer, *dImgBuffer;
char dMemory[1024], iMemory[1024];

void dealWithDImg();
void dealWithIImg();
void run();

int main() {
	FILE *iImg = fopen("iimage.bin", "rb");
	FILE *dImg = fopen("dimage.bin", "rb");
	
	// Load the files.
	if (iImg == NULL || dImg == NULL)
		fputs("Please put iimage.bin and dimage.bin here.\n", stderr), exit(1);
	
	// Get the length of the files.
	fseek(iImg, 0, SEEK_END), fseek(dImg, 0, SEEK_END);
	iImgLen = ftell(iImg), dImgLen = ftell(dImg);
	
	// Move the pointers to the beginning of the files.
	rewind(iImg), rewind(dImg);
	
	// Allocate memory to contain the files.
	iImgBuffer = (char *) malloc(sizeof(char) * iImgLen);
	dImgBuffer = (char *) malloc(sizeof(char) * dImgLen);
	if (iImgBuffer == NULL || dImgBuffer == NULL)
		fputs("Memory allocation error.\n", stderr), exit(2);
	
	// Copy the file to the buffers.
	iImgLenResult = fread(iImgBuffer, 1, iImgLen, iImg);
	dImgLenResult = fread(dImgBuffer, 1, dImgLen, dImg);
	if (iImgLen != iImgLenResult || dImgLen != dImgLenResult)
		fputs("Reading error.\n", stderr), exit(3);
	
	// Close the files.
	fclose(iImg), fclose(dImg);
	
	dealWithDImg();
	
	dealWithIImg();
	
	run();
	
	return 0;
}

void dealWithDImg() {
	unsigned i, temp = 0, idx = 0;
	// Get the value of $sp.
	for (i = 0; i < 4; i++)
		temp = (temp << 8) + dImgBuffer[i];
	reg[$sp] = temp;
	// Get the number for D memory.
	temp = 0;
	for (i = 4; i < 8; i++)
		temp = (temp << 8) + dImgBuffer[i];
	// Write the value to D memory.
	for (i = 8; i < 8 + 4 * temp; i++)
		dMemory[idx++] = dImgBuffer[i];
}

void dealWithIImg() {
	unsigned i, temp = 0, idx = 0;
	// Get the value of PC.
	for (i = 0; i < 4; i++)
		temp = (temp << 8) + iImgBuffer[i];
	PC = temp;
	// Get the number for I memory.
	temp = 0;
	for (i = 4; i < 8; i++)
		temp = (temp << 8) + iImgBuffer[i];
	// Write the value to I memory.
	for (i = 8; i < 8 + 4 * temp; i++)
		iMemory[idx++] = iImgBuffer[i];
}

void run() {
	unsigned opcode = 0;
	// Deal with the first instruction.
	opcode = iMemory[0];
	opcode = opcode >> 2 << 26 >> 26;
	// Loop while opcode != halt
	while (opcode != halt) {
		opcode = iMemory[34 * 4];
		opcode = opcode >> 2 << 26 >> 26;
	}
}