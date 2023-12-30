//
// Created by alena on 29.12.2023.
//

#ifndef SEM_CHECKSUMA_H
#define SEM_CHECKSUMA_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#define SIGNATURE_SIZE 3 // bytes
#define CRC32_SIZE 4     // bytes

void setLSBSignature(unsigned char* byte, int bitValue);
void generateSignature(uint8_t signature[], size_t length);
int checkSignature(const Pixel* pixels, const uint8_t* signature);
void embedSignature(Pixel* pixels, const uint8_t* signature, int signatureSize);
void extractSignature(const Pixel* pixels, uint8_t* extractedSignature, int signatureSize);
void testSignatureEmbedExtract();
#endif //SEM_CHECKSUMA_H
