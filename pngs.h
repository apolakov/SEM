//
// Created by alena on 23.12.2023.
//

#ifndef SEMESTRALKAPLSPLSPLS_PNGS_H
#define SEMESTRALKAPLSPLSPLS_PNGS_H

#include <png.h>
#include "bmp.h"


int readPNG(const char* filename, Pixel** outPixels, int* outWidth, int* outHeight);

int writePNG(const char* filename, Pixel* pixels, int width, int height);

//int embedPayloadInPixels(Pixel* pixels, int width, int height, const unsigned char* payload, size_t payloadSize);

void embedBit(Pixel* pixel, size_t bit);

int writePNG(const char* filename, Pixel* pixels, int width, int height);

//int embedPayloadInPNG(const char* imageFilename, const char* outputImageFilename, const int* compressedPayload, int compressedSize);

int extractAndDecompressPayloadFromPNG(const char* inputImageFilename, const char* outputPayloadFilename);

//int extractPayloadFromPixels(const Pixel* pixels, int width, int height, int** outCompressedPayload, int* outCompressedSize);

unsigned int extractSizeFromPixelDataPNG(const Pixel* pixels);

//void embedSizePNG(Pixel* pixels, unsigned int sizeInBits);
//int embedPayloadInPixels(Pixel* pixels, int width, int height, const unsigned char* payload, size_t payloadByteSize);
//int extractPayloadFromPixels(const Pixel* pixels, int width, int height, int** outCompressedPayload, int* outCompressedSizeBits);
//int extractPayloadFromPixels(const Pixel* pixels, int width, int height, unsigned char** outPayload, size_t* outPayloadSizeBits);
int embedPayloadInPNG(const char* imageFilename, const char* outputImageFilename, const unsigned char* originalPayload, int originalPayloadSize);
        void embedSizePNG(Pixel* pixels, unsigned int sizeInBits);
unsigned int extractBit(const Pixel* pixel);
int embedPayloadInPixels(Pixel* pixels, int width, int height, const int* compressedPayload, int compressedPayloadSize);
int extractPayloadFromPixels(const Pixel* pixels, int width, int height, int** outCompressedPayload, int* outCompressedPayloadSize);


#endif //SEMESTRALKAPLSPLSPLS_PNGS_H
