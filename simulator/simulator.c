#include <stdio.h>
#include <stdlib.h>
#define $sp 29

unsigned iImgLen, dImgLen, iImgLenResult, dImgLenResult;
unsigned reg[32], PC;
char *iImgBuffer, *dImgBuffer;
char dMemory[1024], iMemory[1024];

void dealWithDImg();
void dealWithIImg();

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
	
	
	return 0;
}

void dealWithDImg() {
	unsigned i, temp = 0, idx = 0;
	// Get the value of $sp
	for (i = 0; i < 4; i++)
		temp = (temp << 8) + dImgBuffer[i];
	reg[$sp] = temp;
	// Get the number for D memory
	temp = 0;
	for (i = 4; i < 8; i++)
		temp = (temp << 8) + dImgBuffer[i];
	// Write the value to D memory
	for (i = 8; i < 8 + 4 * temp; i++)
		dMemory[idx++] = dImgBuffer[i];
}

void dealWithIImg() {
	unsigned i, temp = 0, idx = 0;
	// Get the value of PC
	for (i = 0; i < 4; i++)
		temp = (temp << 8) + iImgBuffer[i];
	PC = temp;
	// Get the number for I memory
	temp = 0;
	for (i = 4; i < 8; i++)
		temp = (temp << 8) + iImgBuffer[i];
	// Write the value to I memory
	for (i = 8; i < 8 + 4 * temp; i++)
		iMemory[idx++] = iImgBuffer[i];
}