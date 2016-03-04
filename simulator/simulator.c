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

// Warning: do not shift more than 31 bits.
unsigned iImgLen, dImgLen, iImgLenResult, dImgLenResult;
unsigned reg[32], PC;
char *iImgBuffer, *dImgBuffer;
char dMemory[1024], iMemory[1024];
FILE *err, *snap;

void dealWithDImg();
void dealWithIImg();
void findRsRtRd(unsigned *rs, unsigned *rt, unsigned *rd, unsigned insPos);
void findShamt(unsigned *shamt, unsigned insPos);
void run();

int main() {
	FILE *iImg = fopen("iimage.bin", "rb");
	FILE *dImg = fopen("dimage.bin", "rb");
	err = fopen("error_dump.rpt", "wb");
	snap = fopen("snapshot.rpt", "wb");
	
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

void findRsRtRd(unsigned *rs, unsigned *rt, unsigned *rd, unsigned insPos) {
	// Deal with rs.
	unsigned temp1 = iMemory[insPos], temp2 = iMemory[insPos + 1];
	temp1 = temp1 << 30 >> 27;
	temp2 = temp2 << 24 >> 29;
	*rs = temp1 + temp2;
	// Deal with rt.
	*rt = iMemory[insPos + 1];
	*rt = (*rt) << 27 >> 27; 
	// Deal with rd.
	*rd = iMemory[insPos + 2];
	*rd = (*rd) << 24 >> 27;
}

void findShamt(unsigned *shamt, unsigned insPos) {
	unsigned temp1 = iMemory[insPos + 2], temp2 = iMemory[insPos + 3];
	temp1 = temp1 << 29 >> 27;
	temp2 = temp2 >> 6 << 30 >> 30;
	*shamt = temp1 + temp2;
}

void run() {
	unsigned opcode, insPos = 0, cycle = 0;
	// Deal with the first instruction.
	opcode = iMemory[insPos];
	opcode = opcode >> 2 << 26 >> 26;
	// Loop while opcode != halt.
	while (opcode != halt) {
		// If opcode == R.
		if (opcode == R) {
			unsigned shamt, funct, rs, rt, rd, signRs, signRt, signRd;
			int intRs, intRt, temp;
			// Then load funct.
			funct = iMemory[insPos + 3];
			funct = funct << 26 >> 26;
			// Find rs, rt, rd and then go on.
			findRsRtRd(&rs, &rt, &rd, insPos);
			switch (funct) {
				// Instruction add with overflow detection.
				case add:
					signRs = reg[rs] >> 31, signRt = reg[rt] >> 31;
					reg[rd] = reg[rs] + reg[rt];
					signRd = reg[rd] >> 31;
					if (signRs == signRt && signRs != signRd)
						fprintf(err, "In cycle %d: Number Overflow\n", cycle);
					break;
				case addu:
					reg[rd] = reg[rs] + reg[rt];
					break;
				case sub:
					signRs = reg[rs] >> 31, signRt = (-reg[rt]) >> 31;
					reg[rd] = reg[rs] - reg[rt];
					signRd = reg[rd] >> 31;
					if (signRs == signRt && signRs != signRd)
						fprintf(err, "In cycle %d: Number Overflow\n", cycle);
					break;
				case and:
					reg[rd] = reg[rs] & reg[rt];
					break;
				case or:
					reg[rd] = reg[rs] | reg[rt];
					break;
				case xor:
					reg[rd] = reg[rs] ^ reg[rt];
					break;
				case nor:
					reg[rd] = ~(reg[rs] | reg[rt]);
					break;
				case nand:
					reg[rd] = ~(reg[rs] & reg[rt]);
					break;
				case slt:
					intRs = rs, intRt = rt;
					reg[rd] = (intRs < intRt);
					break;
				case sll:
					findShamt(&shamt, insPos);
					reg[rd] = reg[rt] << shamt;
					break;
				case srl:
					findShamt(&shamt, insPos);
					reg[rd] = reg[rt] >> shamt;
					break;
				case sra:
					findShamt(&shamt, insPos);
					temp = reg[rt];
					temp = temp >> shamt;
					reg[rd] = temp;
					break;
				default:
					PC = rs;
					break;
			}
		}
		insPos += 4;
		opcode = iMemory[insPos];
		opcode = opcode >> 2 << 26 >> 26;
		cycle++;
	}
}