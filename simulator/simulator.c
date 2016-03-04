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
#define lb 32 // test
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
unsigned reg[32], PC, cycle, insPos;
char *iImgBuffer, *dImgBuffer;
char dMemory[1024], iMemory[1024];
FILE *err, *snap;

void dealWithDImg();
void dealWithIImg();
void findRsRtRd(unsigned *rs, unsigned *rt, unsigned *rd, unsigned insPos);
void findShamt(unsigned *shamt, unsigned insPos);
void findUnsignedImmediate(unsigned *immediate, unsigned insPos);
void findSignedImmediate(unsigned *immediate, unsigned insPos);
void run();
void dumpSnap();
int tranPosByShortIm(unsigned *pos, unsigned short *shortIm, unsigned *rs);

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
	
	// Execute the functions.
	dealWithDImg();
	dealWithIImg();
	run();
	
	return 0;
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
	if (rd == NULL) return;
	*rd = iMemory[insPos + 2];
	*rd = (*rd) << 24 >> 27;
}

void findShamt(unsigned *shamt, unsigned insPos) {
	unsigned temp1 = iMemory[insPos + 2], temp2 = iMemory[insPos + 3];
	temp1 = temp1 << 29 >> 27;
	temp2 = temp2 >> 6 << 30 >> 30;
	*shamt = temp1 + temp2;
}

void findUnsignedImmediate(unsigned *immediate, unsigned insPos) {
	unsigned temp1 = iMemory[insPos + 2], temp2 = iMemory[insPos + 3];
	temp1 = temp1 << 24 >> 16;
	temp2 = temp2 << 24 >> 24;
	*immediate = temp1 + temp2;
}

void findSignedImmediate(unsigned *immediate, unsigned insPos) {
	unsigned temp1 = iMemory[insPos + 2], temp2 = iMemory[insPos + 3];
	temp1 = temp1 << 24 >> 16;
	temp2 = temp2 << 24 >> 24;
	int temp = temp1 + temp2;
	temp = temp << 16 >> 16;
	*immediate = temp;
}

int tranPosByShortIm(unsigned *pos, unsigned short *shortIm, unsigned *rs) {
	if (*shortIm >> 15) {
		*shortIm = ~(*shortIm - 1);
		*pos = reg[*rs] - *shortIm;
	} else *pos = reg[*rs] + *shortIm;
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
		//printf("%u\n", opcode);
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
		} else if (opcode == jal) {
			reg[31] = insPos + 4;
			unsigned address, temp1 = iMemory[insPos], temp2 = iMemory[insPos + 1], temp3 = iMemory[insPos + 2], temp4 = iMemory[insPos + 3];
			temp1 = temp1 << 30 >> 30;
			temp2 = temp2 << 24 >> 24;
			temp3 = temp3 << 24 >> 24;
			temp4 = temp4 << 24 >> 24;
			address = temp1 + temp2 + temp3 + temp4;
			insPos = ((insPos + 4) >> 28 << 28) | (address << 2);
		} else {
			if (opcode == R) {
				unsigned shamt, funct, rs, rt, rd, signRs, signRt, signRd;
				int intRs, intRt, temp;
				funct = iMemory[insPos + 3];
				funct = funct << 26 >> 26;
				findRsRtRd(&rs, &rt, &rd, insPos);
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
			} else {
				unsigned rs, rt, immediate, signRs, signRt, signIm;
				findRsRtRd(&rs, &rt, NULL, insPos);
				if (opcode == addi) { 
					findSignedImmediate(&immediate, insPos);
					signRs = reg[rs] >> 31, signIm = immediate >> 15;
					reg[rt] = reg[rs] + immediate;
					signRt = reg[rt] >> 31;
					if (signRs == signIm && signRs != signRt)
						fprintf(err, "In cycle %d: Number Overflow\n", cycle);
				} else if (opcode == addiu) {
					findUnsignedImmediate(&immediate, insPos);
					reg[rt] = reg[rs] + immediate;
				} else if (opcode == lw) {
					findSignedImmediate(&immediate, insPos);
					if (immediate % 4) {
						fprintf(err, "In cycle %d: Misalignment Error\n", cycle);
						break;
					}
					unsigned temp1, temp2, temp3, temp4, pos;
					unsigned short shortIm = immediate;
					if (tranPosByShortIm(&pos, &shortIm, &rs))
						break;
					temp1 = dMemory[pos], temp1 = temp1 << 24;
					temp2 = dMemory[pos + 1], temp2 = temp2 << 24 >> 8;
					temp3 = dMemory[pos + 2], temp3 = temp3 << 24 >> 16;
					temp4 = dMemory[pos + 3], temp4 = temp4 << 24 >> 24;
					reg[rt] = temp1 + temp2 + temp3 + temp4;
				} else if (opcode == lh) {
					findSignedImmediate(&immediate, insPos);
					if (immediate % 2) {
						fprintf(err, "In cycle %d: Misalignment Error\n", cycle);
						break;
					}
					unsigned temp1, temp2, pos;
					unsigned short shortIm = immediate;
					if (tranPosByShortIm(&pos, &shortIm, &rs))
						break;
					temp1 = dMemory[pos], temp1 = temp1 << 24 >> 16;
					temp2 = dMemory[pos + 1], temp2 = temp2 << 24 >> 24;
					short temp3 = temp1 + temp2;
					reg[rt] = temp3;
				} else if (opcode == lhu) {
					findSignedImmediate(&immediate, insPos);
					if (immediate % 2) {
						fprintf(err, "In cycle %d: Misalignment Error\n", cycle);
						break;
					}
					unsigned temp1, temp2, pos;
					pos = reg[rs] + immediate;
					if (pos >= 1024) {
						fprintf(err, "In cycle %d: Address Overflow\n", cycle);
						break;
					}
					temp1 = dMemory[pos], temp1 = temp1 << 24 >> 16;
					temp2 = dMemory[pos + 1], temp2 = temp2 << 24 >> 24;
					reg[rt] = temp1 + temp2;
				} else if (opcode == lb) {
					findSignedImmediate(&immediate, insPos);
					unsigned pos;
					unsigned short shortIm = immediate;
					if (tranPosByShortIm(&pos, &shortIm, &rs))
						break;
					reg[rt] = dMemory[pos];
				} else if (opcode == lbu) {
					findSignedImmediate(&immediate, insPos);
					unsigned pos;
					pos = reg[rs] + immediate;
					if (pos >= 1024) {
						fprintf(err, "In cycle %d: Address Overflow\n", cycle);
						break;
					}
					reg[rt] = dMemory[pos], reg[rt] = reg[rt] << 24 >> 24;
				} else if (opcode == sw) {
					findSignedImmediate(&immediate, insPos);
					unsigned pos;
					unsigned short shortIm = immediate;
					if (tranPosByShortIm(&pos, &shortIm, &rs))
						break;
					dMemory[pos] = reg[rt] >> 24;
					dMemory[pos + 1] = reg[rt] << 8 >> 24;
					dMemory[pos + 2] = reg[rt] << 16 >> 24;
					dMemory[pos + 3] = reg[rt] << 24 >> 24;
				} else if (opcode == sh) {
					findSignedImmediate(&immediate, insPos);
					unsigned pos;
					unsigned short shortIm = immediate;
					if (tranPosByShortIm(&pos, &shortIm, &rs))
						break;
					dMemory[pos] = reg[rt] << 16 >> 24;
					dMemory[pos + 1] = reg[rt] << 24 >> 24;
				} else if (opcode == sb) {
					findSignedImmediate(&immediate, insPos);
					unsigned pos;
					unsigned short shortIm = immediate;
					if (tranPosByShortIm(&pos, &shortIm, &rs))
						break;
					dMemory[pos] = reg[rt] << 24 >> 24;
				} else if (opcode == lui) {
					reg[rt] = immediate << 16;
				} else if (opcode == andi) {
					findUnsignedImmediate(&immediate, insPos);
					reg[rt] = reg[rs] & immediate;
				} else if (opcode == ori) {
					findUnsignedImmediate(&immediate, insPos);
					reg[rt] = reg[rs] | immediate;
				} else if (opcode == nori) {
					findUnsignedImmediate(&immediate, insPos);
					reg[rt] = ~(reg[rs] | immediate);
				} else if (opcode == slti) {
					findSignedImmediate(&immediate, insPos);
					int intIm = immediate, intRs = reg[rs];
					reg[rt] = (intRs < intIm);
				} else if (opcode == beq) {
					findSignedImmediate(&immediate, insPos);
					if (reg[rs] == reg[rt]) {
						unsigned short shortIm = immediate;
						shortIm = shortIm << 2;
						if (shortIm >> 15) insPos -= ~(shortIm - 1);
						else insPos += shortIm;
					}
				} else if (opcode == bne) {
					findSignedImmediate(&immediate, insPos);
					if (reg[rs] != reg[rt]) {
						unsigned short shortIm = immediate;
						shortIm = shortIm << 2;
						if (shortIm >> 15) insPos -= ~(shortIm - 1);
						else insPos += shortIm;
					}
				} else if (opcode == bgtz) {
					findSignedImmediate(&immediate, insPos);
					if (reg[rs] > 0) {
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
		cycle++;
	}
}

void dumpSnap() {
	fprintf(snap, "cycle %u\n", cycle);
	unsigned i;
	char temp;
	for (i = 0; i < 32; i++) {
		fprintf(snap, "$%02u: 0x", i);
		temp = reg[i] >> 24;
		fprintf(snap, "%02X", temp & 0xff);
		temp = reg[i] << 8 >> 24;
		fprintf(snap, "%02X", temp & 0xff);
		temp = reg[i] << 16 >> 24;
		fprintf(snap, "%02X", temp & 0xff);
		temp = reg[i] << 24 >> 24;
		fprintf(snap, "%02X\n", temp & 0xff);
	}
	fprintf(snap, "PC: 0x");
	temp = (PC + insPos) >> 24;
	fprintf(snap, "%02X", temp & 0xff);
	temp = (PC + insPos) << 8 >> 24;
	fprintf(snap, "%02X", temp & 0xff);
	temp = (PC + insPos) << 16 >> 24;
	fprintf(snap, "%02X", temp & 0xff);
	temp = (PC + insPos) << 24 >> 24;
	fprintf(snap, "%02X\n\n\n", temp & 0xff);
}