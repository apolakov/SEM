//
// Created by alena on 22.12.2023.
//

#ifndef SEMESTRALKAPLSPLSPLS_FILES_H
#define SEMESTRALKAPLSPLSPLS_FILES_H
#include "bmp.h"
#include <stdbool.h>



bool is_png(FILE *file);
bool is_bmp(FILE *file);
bool is_24bit_bmp(FILE *file);
bool is_24bit_png(FILE *file);
int determineFileTypeAndCheck24Bit(const char *filename);
int saveImage(const char* filename, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, unsigned char* pixelData, int pixelDataSize);






#endif //SEMESTRALKAPLSPLSPLS_FILES_H
