#include <stdio.h>
#include <stdlib.h>
#include "defines.h"

extern unsigned iImgLen, dImgLen, iImgLenResult, dImgLenResult;
extern unsigned reg[32], PC, cycle, insPos;
extern char *iImgBuffer, *dImgBuffer;
extern char dMemory[1024], iMemory[1024];
extern FILE *err, *snap, *iImg, *dImg;

void dealWithDImg();

void dealWithIImg();

void openNLoadFiles();
