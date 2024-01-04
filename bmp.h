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

int extract_payload(const char* input_name, const char* output_name);
unsigned int extract_size_from_pixels(const Pixel* pixels, int num_pixels);
Pixel* read_pixel_data(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, int* pixel_data_size);
void embed_size(Pixel* pixels, unsigned int size);
void set_lsb(unsigned char* byte, int bitValue);
int* extract_12bit_payload(const Pixel* pixels, int num_pixels, int* compressed_payload_size);
void embed_12bit_payload(Pixel* pixels, int num_pixels, const int* compressed_payload, int compressed_size);
int embed_to_bmp(const char* image_filename, const char* output_image_filename, const int* compressed_payload, int compressed_size, const char* payloadFilename);

#endif
