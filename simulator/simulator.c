#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "defines.h"
#include "decoder.h"
#include "output.h"
#include "load.h"

static unsigned opcode, funct, shamt, address, temp1, temp2, temp3, temp4, rs, rt, rd;
static unsigned immediate, signRs, signRt, signRd, signIm, signPos, pos;
static int intRs, intRt, intIm, temp;

void findOpcode() {
    opcode = iMemory[PC];
    opcode = opcode >> 2 << 26 >> 26;
}

int findPosByImmediateWithErrorDetection(int gap) {
    int needToHalt = 0;
    pos = reg[rs] + immediate;
    signRs = reg[rs] >> 31, signIm = immediate >> 31;
    signPos = pos >> 31;
    if (signRs == signIm && signRs != signPos)
        numberOverflow = 1;
    if (pos + gap >= 1024) {
        memoryOverflow = 1;
        needToHalt = 1;
    }
    if (pos % (gap + 1)) {
        memoryMisalignment = 1;
        needToHalt = 1;
    }
    if (needToHalt) return 1;
    else return 0;
}

void run() {
    while (1) {
        dumpSnap();
        findOpcode();
        switch (opcode) {
            case HALT: return;
            case J:
                temp1 = iMemory[PC];
                temp2 = iMemory[PC + 1];
                temp3 = iMemory[PC + 2];
                temp4 = iMemory[PC + 3];
                temp1 = temp1 << 30 >> 6;
                temp2 = temp2 << 24 >> 8;
                temp3 = temp3 << 24 >> 16;
                temp4 = temp4 << 24 >> 24;
                address = temp1 + temp2 + temp3 + temp4;
                PC = ((PC + 4) >> 28 << 28) | (address << 2);
                break;
            case JAL:
                reg[31] = PC + 4;
                temp1 = iMemory[PC];
                temp2 = iMemory[PC + 1];
                temp3 = iMemory[PC + 2];
                temp4 = iMemory[PC + 3];
                temp1 = temp1 << 30 >> 6;
                temp2 = temp2 << 24 >> 8;
                temp3 = temp3 << 24 >> 16;
                temp4 = temp4 << 24 >> 24;
                address = temp1 + temp2 + temp3 + temp4;
                PC = ((PC + 4) >> 28 << 28) | (address << 2);
                break;
            case R:
                funct = iMemory[PC + 3];
                funct = funct << 26 >> 26;
                findRsRtRd(&rs, &rt, &rd);
                switch (funct) {
                    case ADD:
                        reg[rd] = reg[rs] + reg[rt];
                        if (rd == 0) {
                            writeToZero = 1;
                            reg[rd] = 0;
                        }
                        signRs = reg[rs] >> 31, signRt = reg[rt] >> 31;
                        signRd = reg[rd] >> 31;
                        if (signRs == signRt && signRs != signRd)
                            numberOverflow = 1;
                        PC += 4;
                        break;
                    case ADDU:
                        reg[rd] = reg[rs] + reg[rt];
                        if (rd == 0) {
                            writeToZero = 1;
                            reg[rd] = 0;
                        }
                        PC += 4;
                        break;
                    case SUB:
                        reg[rd] = reg[rs] - reg[rt];
                        if (rd == 0) {
                            writeToZero = 1;
                            reg[rd] = 0;
                        }
                        signRs = reg[rs] >> 31, signRt = (-reg[rt]) >> 31;
                        signRd = reg[rd] >> 31;
                        if (signRs == signRt && signRs != signRd)
                            numberOverflow = 1;
                        PC += 4;
                        break;
                    case AND:
                        reg[rd] = reg[rs] & reg[rt];
                        if (rd == 0) {
                            writeToZero = 1;
                            reg[rd] = 0;
                        }
                        PC += 4;
                        break;
                    case OR:
                        reg[rd] = reg[rs] | reg[rt];
                        if (rd == 0) {
                            writeToZero = 1;
                            reg[rd] = 0;
                        }
                        PC += 4;
                        break;
                    case XOR:
                        reg[rd] = reg[rs] ^ reg[rt];
                        if (rd == 0) {
                            writeToZero = 1;
                            reg[rd] = 0;
                        }
                        PC += 4;
                        break;
                    case NOR:
                        reg[rd] = ~(reg[rs] | reg[rt]);
                        if (rd == 0) {
                            writeToZero = 1;
                            reg[rd] = 0;
                        }
                        PC += 4;
                        break;
                    case NAND:
                        reg[rd] = ~(reg[rs] & reg[rt]);
                        if (rd == 0) {
                            writeToZero = 1;
                            reg[rd] = 0;
                        }
                        PC += 4;
                        break;
                    case SLT:
                        intRs = reg[rs], intRt = reg[rt];
                        reg[rd] = (intRs < intRt);
                        if (rd == 0) {
                            writeToZero = 1;
                            reg[rd] = 0;
                        }
                        PC += 4;
                        break;
                    case SLL:
                        findShamt(&shamt);
                        reg[rd] = reg[rt] << shamt;
                        if (rd == 0) {
                            if (!(rt == 0 && shamt == 0)) {
                                writeToZero = 1;
                                reg[rd] = 0;
                            }
                        }
                        PC += 4;
                        break;
                    case SRL:
                        findShamt(&shamt);
                        reg[rd] = reg[rt] >> shamt;
                        if (rd == 0) {
                            writeToZero = 1;
                            reg[rd] = 0;
                        }
                        PC += 4;
                        break;
                    case SRA:
                        findShamt(&shamt);
                        temp = reg[rt];
                        temp = temp >> shamt;
                        reg[rd] = temp;
                        if (rd == 0) {
                            writeToZero = 1;
                            reg[rd] = 0;
                        }
                        PC += 4;
                        break;
                    default:
                        PC = reg[rs];
                }
                break;
            default:
                findRsRtRd(&rs, &rt, NULL);
                switch (opcode) {
                    case ADDI:
                        findSignedImmediate(&immediate);
                        reg[rt] = reg[rs] + immediate;
                        if (rt == 0) {
                            writeToZero = 1;
                            reg[rt] = 0;
                        }
                        signRs = reg[rs] >> 31, signIm = immediate >> 31;
                        signRt = reg[rt] >> 31;
                        if (signRs == signIm && signRs != signRt)
                            numberOverflow = 1;
                        PC += 4;
                        break;
                    case ADDIU:
                        findUnsignedImmediate(&immediate);
                        reg[rt] = reg[rs] + immediate;
                        if (rt == 0) {
                            writeToZero = 1;
                            reg[rt] = 0;
                        }
                        PC += 4;
                        break;
                    case LW:
                        findSignedImmediate(&immediate);
                        if (findPosByImmediateWithErrorDetection(3)) return;
                        // Misalignment detection is embedded in the findPos function.
                        temp1 = dMemory[pos], temp1 = temp1 << 24;
                        temp2 = dMemory[pos + 1], temp2 = temp2 << 24 >> 8;
                        temp3 = dMemory[pos + 2], temp3 = temp3 << 24 >> 16;
                        temp4 = dMemory[pos + 3], temp4 = temp4 << 24 >> 24;
                        reg[rt] = temp1 + temp2 + temp3 + temp4;
                        if (rt == 0) {
                            writeToZero = 1;
                            reg[rt] = 0;
                        }
                        PC += 4;
                        break;
                    case LH:
                        findSignedImmediate(&immediate);
                        if (findPosByImmediateWithErrorDetection(1)) return;
                        // Misalignment detection is embedded in the findPos function.
                        temp1 = dMemory[pos], temp1 = temp1 << 24 >> 16;
                        temp2 = dMemory[pos + 1], temp2 = temp2 << 24 >> 24;
                        short shortTemp = temp1 + temp2;
                        reg[rt] = shortTemp;
                        if (rt == 0) {
                            writeToZero = 1;
                            reg[rt] = 0;
                        }
                        PC += 4;
                        break;
                    case LHU:
                        findSignedImmediate(&immediate);
                        if (findPosByImmediateWithErrorDetection(1)) return;
                        // Misalignment detection is embedded in the findPos function.
                        temp1 = dMemory[pos], temp1 = temp1 << 24 >> 16;
                        temp2 = dMemory[pos + 1], temp2 = temp2 << 24 >> 24;
                        reg[rt] = temp1 + temp2;
                        if (rt == 0) {
                            writeToZero = 1;
                            reg[rt] = 0;
                        }
                        PC += 4;
                        break;
                    case LB:
                        findSignedImmediate(&immediate);
                        if (findPosByImmediateWithErrorDetection(0)) return;
                        // No need to detect misalignment.
                        reg[rt] = dMemory[pos];
                        if (rt == 0) {
                            writeToZero = 1;
                            reg[rt] = 0;
                        }
                        PC += 4;
                        break;
                    case LBU:
                        findSignedImmediate(&immediate);
                        if (findPosByImmediateWithErrorDetection(0)) return;
                        // No need to detect misalignment.
                        reg[rt] = dMemory[pos], reg[rt] = reg[rt] << 24 >> 24;
                        if (rt == 0) {
                            writeToZero = 1;
                            reg[rt] = 0;
                        }
                        PC += 4;
                        break;
                    case SW:
                        findSignedImmediate(&immediate);
                        if (findPosByImmediateWithErrorDetection(3)) return;
                        // Misalignment detection is embedded in the findPos function.
                        dMemory[pos] = reg[rt] >> 24;
                        dMemory[pos + 1] = reg[rt] << 8 >> 24;
                        dMemory[pos + 2] = reg[rt] << 16 >> 24;
                        dMemory[pos + 3] = reg[rt] << 24 >> 24;
                        PC += 4;
                        break;
                    case SH:
                        findSignedImmediate(&immediate);
                        if (findPosByImmediateWithErrorDetection(1)) return;
                        // Misalignment detection is embedded in the findPos function.
                        dMemory[pos] = reg[rt] << 16 >> 24;
                        dMemory[pos + 1] = reg[rt] << 24 >> 24;
                        PC += 4;
                        break;
                    case SB:
                        findSignedImmediate(&immediate);
                        if (findPosByImmediateWithErrorDetection(0)) return;
                        // No need to detect misalignment.
                        dMemory[pos] = reg[rt] << 24 >> 24;
                        PC += 4;
                        break;
                    case LUI:
                        findUnsignedImmediate(&immediate);
                        reg[rt] = immediate << 16;
                        if (rt == 0) {
                            writeToZero = 1;
                            reg[rt] = 0;
                        }
                        PC += 4;
                        break;
                    case ANDI:
                        findUnsignedImmediate(&immediate);
                        reg[rt] = reg[rs] & immediate;
                        if (rt == 0) {
                            writeToZero = 1;
                            reg[rt] = 0;
                        }
                        PC += 4;
                        break;
                    case ORI:
                        findUnsignedImmediate(&immediate);
                        reg[rt] = reg[rs] | immediate;
                        if (rt == 0) {
                            writeToZero = 1;
                            reg[rt] = 0;
                        }
                        PC += 4;
                        break;
                    case NORI:
                        findUnsignedImmediate(&immediate);
                        reg[rt] = ~(reg[rs] | immediate);
                        if (rt == 0) {
                            writeToZero = 1;
                            reg[rt] = 0;
                        }
                        PC += 4;
                        break;
                    case SLTI:
                        findSignedImmediate(&immediate);
                        intIm = immediate, intRs = reg[rs];
                        reg[rt] = (intRs < intIm) ? 1 : 0;
                        if (rt == 0) {
                            writeToZero = 1;
                            reg[rt] = 0;
                        }
                        PC += 4;
                        break;
                    case BEQ:
                        findSignedImmediate(&immediate);
                        if (reg[rs] == reg[rt]) {
                            immediate = immediate << 2;
                            PC = PC + 4 + immediate;
                        } else PC += 4;
                        break;
                    case BNE:
                        findSignedImmediate(&immediate);
                        if (reg[rs] != reg[rt]) {
                            immediate = immediate << 2;
                            PC = PC + 4 + immediate;
                        } else PC += 4;
                        break;
                    default:
                        findSignedImmediate(&immediate);
                        int temp = reg[rs];
                        if (temp > 0) {
                            immediate = immediate << 2;
                            PC = PC + 4 + immediate;
                        } else PC += 4;
                }
        }
        errorDump();
    }
}

int main() {
    openNLoadFiles();
    dealWithDImg();
    dealWithIImg();
    run();
    // Last return may be an error, so it's necessary to run errorDump() again.
    errorDump();
    return 0;
}
