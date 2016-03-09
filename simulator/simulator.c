#include <stdio.h>
#include <stdlib.h>

#define $sp 29
// Define R's
#define R 0
#define add 32   // used
#define addu 33  // used
#define sub 34   // used
#define and 36   // used
#define or 37    // used
#define xor 38   // used
#define nor 39   // used
#define nand 40  // used
#define slt 42   // used
#define sll 0    // used
#define srl 2    // used
#define sra 3    // used
#define jr 8     // used
// Define others'
#define addi 8   // used
#define addiu 9  // used
#define lw 35    // used
#define lh 33    // used
#define lhu 37   // used
#define lb 32    // used
#define lbu 36   // used
#define sw 43    // used
#define sh 41    // used
#define sb 40    // used
#define lui 15   // used
#define andi 12  // used
#define ori 13   // used
#define nori 14  // used
#define slti 10  // used
#define beq 4    // used
#define bne 5    // used
#define bgtz 7   // used
#define j 2      // used
#define jal 3    // used
#define halt 63  // used

// Warning: do not shift more than 31 bits.
unsigned iImgLen, dImgLen, iImgLenResult, dImgLenResult;
unsigned reg[32], PC, cycle, insPos;
char *iImgBuffer, *dImgBuffer;
char dMemory[1024], iMemory[1024];
FILE *err, *snap, *iImg, *dImg;

void dealWithDImg();
void dealWithIImg();
void findRsRtRd(unsigned *rs, unsigned *rt, unsigned *rd);
void findShamt(unsigned *shamt);
void findUnsignedImmediate(unsigned *immediate);
void findSignedImmediate(unsigned *immediate);
void run();
void dumpSnap();
void openNLoadFiles();
int findPosByImmediateWithMemOverflowDection(unsigned *pos, unsigned *immediate, unsigned *rs);

int main() {
	openNLoadFiles();
	dealWithDImg();
	dealWithIImg();
	run();
	return 0;
}

void openNLoadFiles() {
	// Open the files.
	iImg = fopen("iimage.bin", "rb");
	dImg = fopen("dimage.bin", "rb");
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
}

void dealWithDImg() {
	unsigned i, temp = 0, idx = 0;
	// Get the value of $sp.
	for (i = 0; i < 4; i++)
		temp = (temp << 8) + (unsigned char) dImgBuffer[i];
	reg[$sp] = temp;
	// Get the number for D memory.
	temp = 0;
	for (i = 4; i < 8; i++)
		temp = (temp << 8) + (unsigned char) dImgBuffer[i];
	// Write the value to D memory.
	for (i = 8; i < 8 + 4 * temp; i++)
		dMemory[idx++] = dImgBuffer[i];
}

void dealWithIImg() {
	unsigned i, temp = 0, idx = 0;
	// Get the value of PC.
	for (i = 0; i < 4; i++)
		temp = (temp << 8) + (unsigned char) iImgBuffer[i];
	PC = temp;
	// Get the number for I memory.
	temp = 0;
	for (i = 4; i < 8; i++)
		temp = (temp << 8) + (unsigned char) iImgBuffer[i];
	// Write the value to I memory.
	for (i = 8; i < 8 + 4 * temp; i++)
		iMemory[idx++] = iImgBuffer[i];
}

void findRsRtRd(unsigned *rs, unsigned *rt, unsigned *rd) {
	// Deal with rs.
	unsigned temp1 = iMemory[insPos], temp2 = iMemory[insPos + 1];
	temp1 = temp1 << 30 >> 27;
	temp2 = temp2 << 24 >> 29;
	*rs = temp1 + temp2;
	// Deal with rt.
	*rt = iMemory[insPos + 1];
	*rt = (*rt) << 27 >> 27; 
	// Deal with rd.
	if (rd == NULL) return;
	*rd = iMemory[insPos + 2];
	*rd = (*rd) << 24 >> 27;
}

void findShamt(unsigned *shamt) {
	unsigned temp1 = iMemory[insPos + 2], temp2 = iMemory[insPos + 3];
	temp1 = temp1 << 29 >> 27;
	temp2 = temp2 >> 6 << 30 >> 30;
	*shamt = temp1 + temp2;
}

void findUnsignedImmediate(unsigned *immediate) {
	unsigned temp1 = iMemory[insPos + 2], temp2 = iMemory[insPos + 3];
	temp1 = temp1 << 24 >> 16;
	temp2 = temp2 << 24 >> 24;
	*immediate = temp1 + temp2;
}

void findSignedImmediate(unsigned *immediate) {
	unsigned temp1 = iMemory[insPos + 2], temp2 = iMemory[insPos + 3];
	temp1 = temp1 << 24 >> 16;
	temp2 = temp2 << 24 >> 24;
	int temp = temp1 + temp2;
	temp = temp << 16 >> 16;
	*immediate = temp;
}

int findPosByImmediateWithMemOverflowDection(unsigned *pos, unsigned *immediate, unsigned *rs) {
	*pos = reg[*rs] + *immediate;
	if (*pos >= 1024) {
		fprintf(err, "In cycle %d: Address Overflow\n", cycle);
		return 1;
	} 
	return 0;
}

void run() {
	unsigned opcode;
	// Deal with the first instruction.
	opcode = iMemory[insPos];
	opcode = opcode >> 2 << 26 >> 26;
	while (1) {
		//printf("opcode: %u\n", opcode);
		dumpSnap();
		if (opcode == halt) {
			break;
		} else if (opcode == j) {
			unsigned address, temp1 = iMemory[insPos], temp2 = iMemory[insPos + 1], temp3 = iMemory[insPos + 2], temp4 = iMemory[insPos + 3];
			temp1 = temp1 << 30 >> 30;
			temp2 = temp2 << 24 >> 24;
			temp3 = temp3 << 24 >> 24;
			temp4 = temp4 << 24 >> 24;
			address = temp1 + temp2 + temp3 + temp4;
			insPos = ((insPos + 4) >> 28 << 28) | (address << 2);
			// -4 because there's a +4 at the end of this loop.
			insPos -= PC + 4;
		} else if (opcode == jal) {
			reg[31] = insPos + PC + 4;
			unsigned address, temp1 = iMemory[insPos], temp2 = iMemory[insPos + 1], temp3 = iMemory[insPos + 2], temp4 = iMemory[insPos + 3];
			temp1 = temp1 << 30 >> 30;
			temp2 = temp2 << 24 >> 24;
			temp3 = temp3 << 24 >> 24;
			temp4 = temp4 << 24 >> 24;
			address = temp1 + temp2 + temp3 + temp4;
			insPos = ((insPos + 4) >> 28 << 28) | (address << 2);
			// -4 because there's a +4 at the end of this loop.
			insPos -= PC + 4;
		} else {
			if (opcode == R) {
				unsigned shamt, funct, rs, rt, rd, signRs, signRt, signRd;
				int intRs, intRt, temp;
				funct = iMemory[insPos + 3];
				funct = funct << 26 >> 26;
				findRsRtRd(&rs, &rt, &rd);
				//printf("funct: %u\n\n", funct);
				switch (funct) {
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
						intRs = reg[rs], intRt = reg[rt];
						reg[rd] = (intRs < intRt);
						break;
					case sll:
						findShamt(&shamt);
						reg[rd] = reg[rt] << shamt;
						break;
					case srl:
						findShamt(&shamt);
						reg[rd] = reg[rt] >> shamt;
						break;
					case sra:
						findShamt(&shamt);
						temp = reg[rt];
						temp = temp >> shamt;
						reg[rd] = temp;
						break;
					default:
						// -4 because there's a +4 at the end of this loop.
						insPos = reg[rs] - PC - 4;
						break;
				}
			} else {
				unsigned rs, rt, immediate, signRs, signRt, signIm;
				findRsRtRd(&rs, &rt, NULL);
				if (opcode == addi) { 
					findSignedImmediate(&immediate);
					signRs = reg[rs] >> 31, signIm = immediate >> 31;
					reg[rt] = reg[rs] + immediate;
					signRt = reg[rt] >> 31;
					if (signRs == signIm && signRs != signRt)
						fprintf(err, "In cycle %d: Number Overflow\n", cycle);
				} else if (opcode == addiu) {
					findUnsignedImmediate(&immediate);
					reg[rt] = reg[rs] + immediate;
				} else if (opcode == lw) {
					findSignedImmediate(&immediate);
					if (immediate % 4) {
						fprintf(err, "In cycle %d: Misalignment Error\n", cycle);
						break;
					}
					unsigned temp1, temp2, temp3, temp4, pos;
					
					if (findPosByImmediateWithMemOverflowDection(&pos, &immediate, &rs)) break;
					
					if (rt == 0) fprintf(err, "In cycle %d: Write $0 Error\n", cycle);
					else {
						temp1 = dMemory[pos], temp1 = temp1 << 24;
						temp2 = dMemory[pos + 1], temp2 = temp2 << 24 >> 8;
						temp3 = dMemory[pos + 2], temp3 = temp3 << 24 >> 16;
						temp4 = dMemory[pos + 3], temp4 = temp4 << 24 >> 24;
						reg[rt] = temp1 + temp2 + temp3 + temp4;
					}
				} else if (opcode == lh) {
					findSignedImmediate(&immediate);
					if (immediate % 2) {
						fprintf(err, "In cycle %d: Misalignment Error\n", cycle);
						break;
					}
					unsigned temp1, temp2, pos;
					
					if (findPosByImmediateWithMemOverflowDection(&pos, &immediate, &rs)) break;
					
					if (rt == 0) fprintf(err, "In cycle %d: Write $0 Error\n", cycle);
					else {
						temp1 = dMemory[pos], temp1 = temp1 << 24 >> 16;
						temp2 = dMemory[pos + 1], temp2 = temp2 << 24 >> 24;
						short temp3 = temp1 + temp2;
						reg[rt] = temp3;
					}
				} else if (opcode == lhu) {
					findSignedImmediate(&immediate);
					if (immediate % 2) {
						fprintf(err, "In cycle %d: Misalignment Error\n", cycle);
						break;
					}
					unsigned temp1, temp2, pos;
					
					if (findPosByImmediateWithMemOverflowDection(&pos, &immediate, &rs)) break;
					
					if (rt == 0) fprintf(err, "In cycle %d: Write $0 Error\n", cycle);
					else {
						temp1 = dMemory[pos], temp1 = temp1 << 24 >> 16;
						temp2 = dMemory[pos + 1], temp2 = temp2 << 24 >> 24;
						reg[rt] = temp1 + temp2;
					}
				} else if (opcode == lb) {
					findSignedImmediate(&immediate);
					unsigned pos;
					
					if (findPosByImmediateWithMemOverflowDection(&pos, &immediate, &rs)) break;
					
					if (rt == 0) fprintf(err, "In cycle %d: Write $0 Error\n", cycle);
					else reg[rt] = dMemory[pos];
				} else if (opcode == lbu) {
					findSignedImmediate(&immediate);
					unsigned pos;
					
					if (findPosByImmediateWithMemOverflowDection(&pos, &immediate, &rs)) break;
					
					if (rt == 0) fprintf(err, "In cycle %d: Write $0 Error\n", cycle);
					else reg[rt] = dMemory[pos], reg[rt] = reg[rt] << 24 >> 24;
				} else if (opcode == sw) {
					findSignedImmediate(&immediate);
					unsigned pos;
					
					if (findPosByImmediateWithMemOverflowDection(&pos, &immediate, &rs)) break;
					
					dMemory[pos] = reg[rt] >> 24;
					dMemory[pos + 1] = reg[rt] << 8 >> 24;
					dMemory[pos + 2] = reg[rt] << 16 >> 24;
					dMemory[pos + 3] = reg[rt] << 24 >> 24;
				} else if (opcode == sh) {
					findSignedImmediate(&immediate);
					unsigned pos;
					
					if (findPosByImmediateWithMemOverflowDection(&pos, &immediate, &rs)) break;
					
					dMemory[pos] = reg[rt] << 16 >> 24;
					dMemory[pos + 1] = reg[rt] << 24 >> 24;
				} else if (opcode == sb) {
					findSignedImmediate(&immediate);
					unsigned pos;
					
					if (findPosByImmediateWithMemOverflowDection(&pos, &immediate, &rs)) break;
					
					dMemory[pos] = reg[rt] << 24 >> 24;
				} else if (opcode == lui) {
					findUnsignedImmediate(&immediate);
					reg[rt] = immediate << 16;
				} else if (opcode == andi) {
					findUnsignedImmediate(&immediate);
					reg[rt] = reg[rs] & immediate;
				} else if (opcode == ori) {
					findUnsignedImmediate(&immediate);
					reg[rt] = reg[rs] | immediate;
				} else if (opcode == nori) {
					findUnsignedImmediate(&immediate);
					reg[rt] = ~(reg[rs] | immediate);
				} else if (opcode == slti) {
					findSignedImmediate(&immediate);
					int intIm = immediate, intRs = reg[rs];
					reg[rt] = (intRs < intIm);
				} else if (opcode == beq) {
					findSignedImmediate(&immediate);
					if (reg[rs] == reg[rt]) {
						unsigned short shortIm = immediate;
						shortIm = shortIm << 2;
						if (shortIm >> 15) insPos -= ~(shortIm - 1);
						else insPos += shortIm;
					}
				} else if (opcode == bne) {
					findSignedImmediate(&immediate);
					if (reg[rs] != reg[rt]) {
						unsigned short shortIm = immediate;
						shortIm = shortIm << 2;
						if (shortIm >> 15) insPos -= ~(shortIm - 1);
						else insPos += shortIm;
					}
				} else if (opcode == bgtz) {
					findSignedImmediate(&immediate);
					int temp = reg[rs];
					if (temp > 0) {
						unsigned short shortIm = immediate;
						shortIm = shortIm << 2;
						if (shortIm >> 15) insPos -= ~(shortIm - 1);
						else insPos += shortIm;
					}
				}
			}
		}
		insPos += 4;
		opcode = iMemory[insPos];
		opcode = opcode >> 2 << 26 >> 26;
	}
}

void dumpSnap() {
	fprintf(snap, "cycle %u\n", cycle++);
	unsigned i;
	for (i = 0; i < 32; ++i) {
		fprintf(snap, "$%02u: 0x", i);
		fprintf(snap, "%08X\n", reg[i]);
	}
	fprintf(snap, "PC: 0x");
	fprintf(snap, "%08X\n\n\n", PC + insPos);
}