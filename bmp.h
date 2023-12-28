//
// Created by alena on 22.12.2023.
//

#ifndef SEMESTRALKAPLSPLSPLS_BMP_H
#define SEMESTRALKAPLSPLSPLS_BMP_H


#include <stdio.h>

typedef struct {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
} Pixel;




#pragma pack(push, 1)
typedef struct {
    unsigned short type;              // Magic identifier: 0x4d42
    unsigned int size;                // File size in bytes
    unsigned short reserved1;         // Not used
    unsigned short reserved2;         // Not used
    unsigned int offset;              // Offset to image data in bytes from beginning of file
} BITMAPFILEHEADER;

typedef struct {
    unsigned int size;                // Header size in bytes
    int width,height;                 // Width and height of image
    unsigned short planes;            // Number of colour planes
    unsigned short bits;              // Bits per pixel
    unsigned int compression;         // Compression type
    unsigned int imagesize;           // Image size in bytes
    int xresolution,yresolution;      // Pixels per meter
    unsigned int ncolours;            // Number of colours
    unsigned int importantcolours;    // Important colours
} BITMAPINFOHEADER;
#pragma pack(pop)




int embedPayloadInImage(const char* imageFilename, const char* outputImageFilename, const int* compressedPayload, int compressedSize, const char* payloadFilename);
const char* getFileExtension(const char* filename);
int extractAndDecompressPayload(const char* inputImageFilename, const char* outputPayloadBaseFilename);
int* extractPayload(const Pixel* pixels, int numPixels, int* compressedPayloadSize);
unsigned int extractSizeFromPixelData(const Pixel* pixels, int numPixels);
Pixel* readPixelData(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, int* pixelDataSize);
//void embedPayload(Pixel* pixels, int numPixels, const int* compressedPayload, int compressedSize);
int embedPayload(Pixel* pixels, int numPixels, const int* compressedPayload, int compressedSize);
int embedFileType(Pixel* pixels, const char* fileType);
void embedSize(Pixel* pixels, unsigned int size);
int extractFileType(Pixel* pixels, char* fileType);
void setLSB(unsigned char* byte, int bitValue);
int getBit(const int* data, int size, int position);
//unsigned short* extractPayload(const Pixel* pixels, int numPixels, int* compressedPayloadSize);
//int getBit(const unsigned short* data, int size, int position);





#endif //SEMESTRALKAPLSPLSPLS_BMP_H