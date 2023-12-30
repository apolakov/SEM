//
// Created by alena on 29.12.2023.
//

#ifndef SEM_CHECKSUMA_H
#define SEM_CHECKSUMA_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "bmp.h"
#include <string.h>


#define SIGNATURE_SIZE 3 // bytes
#define CRC32_SIZE 4     // bytes

void testCRC32();
void setLSBSignature(uint8_t* byte, uint8_t bitValue);
void generateSignature(uint8_t signature[], size_t length);
int checkSignature(const Pixel* pixels, const uint8_t* signature);
void embedSignature(Pixel* pixels);
void extractSignature(const Pixel* pixels, uint8_t* extractedSignature, int signatureSize);
void testSignatureEmbedExtract();
uint32_t calculateCRC32(const uint8_t* data, size_t length);
void generate_crc32_table();
uint32_t extractCRC(const Pixel* pixels, int crcPosition);
int extractAndCheckSignature(const Pixel* pixels);


#endif //SEM_CHECKSUMA_H
