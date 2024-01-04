//
// Created by alena on 22.12.2023.
//

#ifndef SEMESTRALKAPLSPLSPLS_FILES_H
#define SEMESTRALKAPLSPLSPLS_FILES_H
#include "bmp.h"


int determine_type(const char *filename);
int is_png(FILE *file);
int is_bmp(FILE *file);
int is_24bit_bmp(FILE *file);
int is_24bit_png(FILE *file);
int save_image(const char* filename, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, unsigned char* pixelData);





#endif //SEMESTRALKAPLSPLSPLS_FILES_H
