//
// Created by alena on 23.12.2023.
//

#ifndef SEMESTRALKAPLSPLSPLS_PNGS_H
#define SEMESTRALKAPLSPLSPLS_PNGS_H

#include <png.h>

typedef struct {
    unsigned char red, green, blue;
} PNGPixel;



void print_pixel_data(PNGPixel* pixels, png_uint_32 width, png_uint_32 height);
PNGPixel* read_png_file(const char* file_name, png_uint_32* width, png_uint_32* height);
#endif //SEMESTRALKAPLSPLSPLS_PNGS_H
