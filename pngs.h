//
// Created by alena on 23.12.2023.
//

#ifndef SEMESTRALKAPLSPLSPLS_PNGS_H
#define SEMESTRALKAPLSPLSPLS_PNGS_H

#include <png.h>
#include "bmp.h"


int read_png(const char* filename, Pixel** out_pixels, int* out_width, int* out_height);
int write_png(const char* filename, Pixel* pixels, int width, int height);
void embed_bit(Pixel* pixel, size_t bit);
int extract_for_png(const char* input_name, const char* output_name);
unsigned int extract_size_png(const Pixel* pixels);
//int embed_to_png(const char* imageFilename, const char* outputImageFilename, const unsigned char* originalPayload, int originalPayloadSize);
unsigned int extract_bit(const Pixel* pixel);
int embed_12bit_png(Pixel* pixels, int width, int height, const int* compressed_payload, int compressed_payload_size);
int extract_pixels_payload(const Pixel* pixels, int width, int height, int** out_compressed_payload, int* out_compressed_size);
void embed_size_png(Pixel* pixels, unsigned int size_in_bits);
int embed_to_png(const char* image_filename, const char* output_name, int* compressed_payload, int compressed_payload_size);
#endif //SEMESTRALKAPLSPLSPLS_PNGS_H
