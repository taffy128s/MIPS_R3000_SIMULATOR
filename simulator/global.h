#include <stdio.h>
#include <stdlib.h>

extern unsigned writeToZero, numberOverflow, memoryOverflow, memoryMisalignment;
extern unsigned iImgLen, dImgLen, iImgLenResult, dImgLenResult, reg[32], PC, cycle;
extern char *iImgBuffer, *dImgBuffer;
extern char dMemory[1024], iMemory[1024];
extern FILE *err, *snap, *iImg, *dImg;

unsigned writeToZero, numberOverflow, memoryOverflow, memoryMisalignment;
unsigned iImgLen, dImgLen, iImgLenResult, dImgLenResult, reg[32], PC, cycle;
char *iImgBuffer, *dImgBuffer;
char dMemory[1024], iMemory[1024];
FILE *err, *snap, *iImg, *dImg;
