//
// Created by alena on 22.12.2023.
//

#ifndef SEMESTRALKAPLSPLSPLS_BMP_H
#define SEMESTRALKAPLSPLSPLS_BMP_H

#include <stdio.h>


#include <string.h>

#define SIGNATURE_SIZE_BITS 48
#define CRC_SIZE_BITS 32
#define SIZE_FIELD_BITS 32

typedef struct {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
} Pixel;

#pragma pack(push, 1)
typedef struct {
    unsigned short type;
    unsigned long size;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned long offset;
} BITMAPFILEHEADER;

typedef struct {
    unsigned long size;
    int width, height;
    unsigned short planes;
    unsigned short bits;
    unsigned long compression;
    unsigned long imagesize;
    int xresolution, yresolution;
    unsigned long ncolours;
    unsigned long importantcolours;
} BITMAPINFOHEADER;
#pragma pack(pop)

int extractAndDecompressPayload(const char* inputImageFilename, const char* outputPayloadBaseFilename);
unsigned int extractSizeFromPixelData(const Pixel* pixels, int numPixels);
Pixel* readPixelData(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, int* pixelDataSize);
void embedSize(Pixel* pixels, unsigned int size);
void setLSB(unsigned char* byte, int bitValue);
int* extract12BitPayload(const Pixel* pixels, int numPixels, int* compressedPayloadSize);
void embed12BitPayload(Pixel* pixels, int numPixels, const int* compressedPayload, int compressedSize);
int embedPayloadInImage(const char* imageFilename, const char* outputImageFilename, const int* compressedPayload, int compressedSize, const char* payloadFilename);

#endif
