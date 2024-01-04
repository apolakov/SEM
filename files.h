//
// Created by alena on 22.12.2023.
//

#ifndef SEMESTRALKAPLSPLSPLS_FILES_H
#define SEMESTRALKAPLSPLSPLS_FILES_H
#include "bmp.h"



int is_png(FILE *file);
int is_bmp(FILE *file);
int is_24bit_bmp(FILE *file);
int is_24bit_png(FILE *file);
int determine_type(const char *filename);
int save_image(const char* filename, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, unsigned char* pixelData, int pixelDataSize);
int write_compressed(const char* filename, const unsigned long* compressedPayload, int compressedSize);





#endif //SEMESTRALKAPLSPLSPLS_FILES_H
