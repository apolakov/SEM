//
// Created by alena on 29.12.2023.
//

#ifndef SEM_CHECKSUMA_H
#define SEM_CHECKSUMA_H

#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"
#include <string.h>



void set_lsb_sign(unsigned char* byte, unsigned char bitValue);
void embed_signature(Pixel* pixels);
void generate_crc32_table();
int check_signature(const Pixel* pixels);
void embed_crc(Pixel* pixels, int width, int height, unsigned long crc, unsigned long payloadBitSize);
unsigned long extract_crc(const Pixel* pixels, int width, int height, int compressedSizeBits);
unsigned long calculate_crc(const int* bitArray, int bitArraySize);

#endif //SEM_CHECKSUMA_H
