#include <stdio.h>
#include <stdlib.h>
#include "defines.h"
#include "decoder.h"
#include "memory.h"
#include "dump.h"

int findPosByImmediateWithMemOverflowDetection(unsigned *pos, unsigned *immediate, unsigned *rs) {
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
        if (opcode == HALT) {
            break;
        } else if (opcode == J) {
            unsigned address, temp1 = iMemory[insPos], temp2 = iMemory[insPos + 1], temp3 = iMemory[insPos + 2], temp4 = iMemory[insPos + 3];
            temp1 = temp1 << 30 >> 30;
            temp2 = temp2 << 24 >> 24;
            temp3 = temp3 << 24 >> 24;
            temp4 = temp4 << 24 >> 24;
            address = temp1 + temp2 + temp3 + temp4;
            insPos = ((insPos + 4) >> 28 << 28) | (address << 2);
            // -4 because there's a +4 at the end of this loop.
            insPos -= PC + 4;
        } else if (opcode == JAL) {
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
                    case ADD:
                        signRs = reg[rs] >> 31, signRt = reg[rt] >> 31;
                        reg[rd] = reg[rs] + reg[rt];
                        signRd = reg[rd] >> 31;
                        if (signRs == signRt && signRs != signRd)
                            fprintf(err, "In cycle %d: Number Overflow\n", cycle);
                        break;
                    case ADDU:
                        reg[rd] = reg[rs] + reg[rt];
                        break;
                    case SUB:
                        signRs = reg[rs] >> 31, signRt = (-reg[rt]) >> 31;
                        reg[rd] = reg[rs] - reg[rt];
                        signRd = reg[rd] >> 31;
                        if (signRs == signRt && signRs != signRd)
                            fprintf(err, "In cycle %d: Number Overflow\n", cycle);
                        break;
                    case AND:
                        reg[rd] = reg[rs] & reg[rt];
                        break;
                    case OR:
                        reg[rd] = reg[rs] | reg[rt];
                        break;
                    case XOR:
                        reg[rd] = reg[rs] ^ reg[rt];
                        break;
                    case NOR:
                        reg[rd] = ~(reg[rs] | reg[rt]);
                        break;
                    case NAND:
                        reg[rd] = ~(reg[rs] & reg[rt]);
                        break;
                    case SLT:
                        intRs = reg[rs], intRt = reg[rt];
                        reg[rd] = (intRs < intRt);
                        break;
                    case SLL:
                        findShamt(&shamt);
                        reg[rd] = reg[rt] << shamt;
                        break;
                    case SRL:
                        findShamt(&shamt);
                        reg[rd] = reg[rt] >> shamt;
                        break;
                    case SRA:
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
                if (opcode == ADDI) {
                    findSignedImmediate(&immediate);
                    signRs = reg[rs] >> 31, signIm = immediate >> 31;
                    reg[rt] = reg[rs] + immediate;
                    signRt = reg[rt] >> 31;
                    if (signRs == signIm && signRs != signRt)
                        fprintf(err, "In cycle %d: Number Overflow\n", cycle);
                } else if (opcode == ADDIU) {
                    findUnsignedImmediate(&immediate);
                    reg[rt] = reg[rs] + immediate;
                } else if (opcode == LW) {
                    findSignedImmediate(&immediate);
                    if (immediate % 4) {
                        fprintf(err, "In cycle %d: Misalignment Error\n", cycle);
                        break;
                    }
                    unsigned temp1, temp2, temp3, temp4, pos;

                    if (findPosByImmediateWithMemOverflowDetection(&pos, &immediate, &rs)) break;

                    if (rt == 0) fprintf(err, "In cycle %d: Write $0 Error\n", cycle);
                    else {
                        temp1 = dMemory[pos], temp1 = temp1 << 24;
                        temp2 = dMemory[pos + 1], temp2 = temp2 << 24 >> 8;
                        temp3 = dMemory[pos + 2], temp3 = temp3 << 24 >> 16;
                        temp4 = dMemory[pos + 3], temp4 = temp4 << 24 >> 24;
                        reg[rt] = temp1 + temp2 + temp3 + temp4;
                    }
                } else if (opcode == LH) {
                    findSignedImmediate(&immediate);
                    if (immediate % 2) {
                        fprintf(err, "In cycle %d: Misalignment Error\n", cycle);
                        break;
                    }
                    unsigned temp1, temp2, pos;

                    if (findPosByImmediateWithMemOverflowDetection(&pos, &immediate, &rs)) break;

                    if (rt == 0) fprintf(err, "In cycle %d: Write $0 Error\n", cycle);
                    else {
                        temp1 = dMemory[pos], temp1 = temp1 << 24 >> 16;
                        temp2 = dMemory[pos + 1], temp2 = temp2 << 24 >> 24;
                        short temp3 = temp1 + temp2;
                        reg[rt] = temp3;
                    }
                } else if (opcode == LHU) {
                    findSignedImmediate(&immediate);
                    if (immediate % 2) {
                        fprintf(err, "In cycle %d: Misalignment Error\n", cycle);
                        break;
                    }
                    unsigned temp1, temp2, pos;

                    if (findPosByImmediateWithMemOverflowDetection(&pos, &immediate, &rs)) break;

                    if (rt == 0) fprintf(err, "In cycle %d: Write $0 Error\n", cycle);
                    else {
                        temp1 = dMemory[pos], temp1 = temp1 << 24 >> 16;
                        temp2 = dMemory[pos + 1], temp2 = temp2 << 24 >> 24;
                        reg[rt] = temp1 + temp2;
                    }
                } else if (opcode == LB) {
                    findSignedImmediate(&immediate);
                    unsigned pos;

                    if (findPosByImmediateWithMemOverflowDetection(&pos, &immediate, &rs)) break;

                    if (rt == 0) fprintf(err, "In cycle %d: Write $0 Error\n", cycle);
                    else reg[rt] = dMemory[pos];
                } else if (opcode == LBU) {
                    findSignedImmediate(&immediate);
                    unsigned pos;

                    if (findPosByImmediateWithMemOverflowDetection(&pos, &immediate, &rs)) break;

                    if (rt == 0) fprintf(err, "In cycle %d: Write $0 Error\n", cycle);
                    else reg[rt] = dMemory[pos], reg[rt] = reg[rt] << 24 >> 24;
                } else if (opcode == SW) {
                    findSignedImmediate(&immediate);
                    unsigned pos;

                    if (findPosByImmediateWithMemOverflowDetection(&pos, &immediate, &rs)) break;

                    dMemory[pos] = reg[rt] >> 24;
                    dMemory[pos + 1] = reg[rt] << 8 >> 24;
                    dMemory[pos + 2] = reg[rt] << 16 >> 24;
                    dMemory[pos + 3] = reg[rt] << 24 >> 24;
                } else if (opcode == SH) {
                    findSignedImmediate(&immediate);
                    unsigned pos;

                    if (findPosByImmediateWithMemOverflowDetection(&pos, &immediate, &rs)) break;

                    dMemory[pos] = reg[rt] << 16 >> 24;
                    dMemory[pos + 1] = reg[rt] << 24 >> 24;
                } else if (opcode == SB) {
                    findSignedImmediate(&immediate);
                    unsigned pos;

                    if (findPosByImmediateWithMemOverflowDetection(&pos, &immediate, &rs)) break;

                    dMemory[pos] = reg[rt] << 24 >> 24;
                } else if (opcode == LUI) {
                    findUnsignedImmediate(&immediate);
                    reg[rt] = immediate << 16;
                } else if (opcode == ANDI) {
                    findUnsignedImmediate(&immediate);
                    reg[rt] = reg[rs] & immediate;
                } else if (opcode == ORI) {
                    findUnsignedImmediate(&immediate);
                    reg[rt] = reg[rs] | immediate;
                } else if (opcode == NORI) {
                    findUnsignedImmediate(&immediate);
                    reg[rt] = ~(reg[rs] | immediate);
                } else if (opcode == SLTI) {
                    findSignedImmediate(&immediate);
                    int intIm = immediate, intRs = reg[rs];
                    reg[rt] = (intRs < intIm);
                } else if (opcode == BEQ) {
                    findSignedImmediate(&immediate);
                    if (reg[rs] == reg[rt]) {
                        unsigned short shortIm = immediate;
                        shortIm = shortIm << 2;
                        if (shortIm >> 15) insPos -= ~(shortIm - 1);
                        else insPos += shortIm;
                    }
                } else if (opcode == BNE) {
                    findSignedImmediate(&immediate);
                    if (reg[rs] != reg[rt]) {
                        unsigned short shortIm = immediate;
                        shortIm = shortIm << 2;
                        if (shortIm >> 15) insPos -= ~(shortIm - 1);
                        else insPos += shortIm;
                    }
                } else if (opcode == BGTZ) {
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

int main() {
    openNLoadFiles();
    dealWithDImg();
    dealWithIImg();
    run();
    return 0;
}
