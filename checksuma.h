//
// Created by alena on 29.12.2023.
//

#ifndef SEM_CHECKSUMA_H
#define SEM_CHECKSUMA_H

#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"
#include <string.h>


#define SIGNATURE_SIZE 3 // bytes
#define CRC32_SIZE 4     // bytes

void setLSBSignature(unsigned char* byte, unsigned char bitValue);
void embedSignature(Pixel* pixels);
void generate_crc32_table();
int extractAndCheckSignature(const Pixel* pixels);
void embedCRCInPixels(Pixel* pixels, int width, int height, unsigned long crc, unsigned long payloadBitSize);
unsigned long extractCRCFromPixels(const Pixel* pixels, int width, int height, int compressedSizeBits);
unsigned long calculateCRC32FromBits(const int* bitArray, int bitArraySize);

#endif //SEM_CHECKSUMA_H
