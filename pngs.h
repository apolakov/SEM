//
// Created by alena on 23.12.2023.
//

#ifndef SEMESTRALKAPLSPLSPLS_PNGS_H
#define SEMESTRALKAPLSPLSPLS_PNGS_H

#include <png.h>
#include "bmp.h"


int readPNG(const char* filename, Pixel** outPixels, int* outWidth, int* outHeight);
int writePNG(const char* filename, Pixel* pixels, int width, int height);
void embedBit(Pixel* pixel, size_t bit);
int extractAndDecompressPayloadFromPNG(const char* inputImageFilename, const char* outputPayloadFilename);
unsigned int extractSizeFromPixelDataPNG(const Pixel* pixels);
int embedPayloadInPNG(const char* imageFilename, const char* outputImageFilename, const unsigned char* originalPayload, int originalPayloadSize);
unsigned int extractBit(const Pixel* pixel);
int embedPayloadInPixels(Pixel* pixels, int width, int height, const int* compressedPayload, int compressedPayloadSize);
int extractPayloadFromPixels(const Pixel* pixels, int width, int height, int** outCompressedPayload, int* outCompressedPayloadSize);
void embedSizePNG(Pixel* pixels, unsigned int sizeInBits);
#endif //SEMESTRALKAPLSPLSPLS_PNGS_H
