#include <png.h>
#include <stdlib.h>
#include <stdio.h>
#include "pngs.h"
#include "bmp.h"
#include "lzw.h"
#include <string.h>
#include "checksuma.h"


int embed_to_png(const char* image_filename, const char* output_name, int* compressed_payload, int compressed_payload_size) {
    unsigned long crc;
    Pixel* image_pixels;
    int image_width, image_height;
    size_t payload_in_bytes, payload_in_bites;
    unsigned int test_size;

    if (!image_filename || !output_name || !compressed_payload) {
        fprintf(stderr, "Null pointer passed to parameters.\n");
        return 1;
    }


    crc = calculate_crc(compressed_payload, compressed_payload_size);

    // Read the PNG file and get its pixel data
    if (read_png(image_filename, &image_pixels, &image_width, &image_height) != 0) {
        fprintf(stderr, "Failed to read the PNG file: %s\n", image_filename);
        return 1;
    }

    size_t payload_in_bits = compressed_payload_size * 12; // 12 bits per code
    size_t total_required_bits = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS + payload_in_bits + CRC_SIZE_BITS;
    size_t total_capacity_bits = (size_t)image_width * image_height; // Assuming one bit per pixel
    if (total_required_bits > total_capacity_bits) {
        fprintf(stderr, "Not enough space in the image for the entire payload and additional data.\n");
        free(image_pixels);
        return 3;
    }


    embed_signature(image_pixels);

    payload_in_bytes = (compressed_payload_size * 12 + 7) / 8;
    payload_in_bites = compressed_payload_size * 12;
    embed_size_png(image_pixels, payload_in_bites);

    test_size = extract_size_png(image_pixels);
    if (test_size != payload_in_bites) {
        fprintf(stderr, "Size embedding test failed! Embedded: %zu, Extracted: %u\n", payload_in_bites, test_size);
        free(image_pixels);
        free(compressed_payload);
        return 1;
    }

    if (embed_12bit_png(image_pixels, image_width, image_height, compressed_payload, (int)payload_in_bytes) != 0) {
        fprintf(stderr, "Failed to embed payload into image.\n");
        free(image_pixels);
        free(compressed_payload);
        return 1;
    }

    embed_crc(image_pixels, image_width, image_height, crc, payload_in_bites);

    if (write_png(output_name, image_pixels, image_width, image_height) != 0) {
        fprintf(stderr, "Failed to write the output PNG file: %s\n", output_name);
        free(image_pixels);
        free(compressed_payload);
        return 1;
    }

    free(image_pixels);
    free(compressed_payload);
    printf("Memory cleaned up and function completed successfully.\n");
    return 0;
}


int extract_for_png(const char* input_name, const char* output_name) {
    Pixel* pixels;
    int width, height, compressed_size_bits, decompressed_payload_size;
    int* compressed_payload;
    unsigned long found_crc, calculated_crc;
    unsigned char* decompressed_payload;
    FILE *output;

    if (!input_name || !output_name) {
        fprintf(stderr, "Null pointer passed to parameters.\n");
        return 1;
    }

    if (read_png(input_name, &pixels, &width, &height) != 0) {
        fprintf(stderr, "Failed to read PNG file: %s\n", input_name);
        return 1;
    }

    if (!check_signature(pixels)) {
        fprintf(stderr, "Signature mismatch or not found in PNG.\n");
        free(pixels);
        return 4;
    }

    if (extract_pixels_payload(pixels, width, height, &compressed_payload, &compressed_size_bits) != 0) {
        fprintf(stderr, "Failed to extract payload from image.\n");
        free(pixels);
        return 1;
    }

    found_crc = extract_crc(pixels, width, height, compressed_size_bits * 12);
    calculated_crc = calculate_crc(compressed_payload, compressed_size_bits);

    if (found_crc != calculated_crc) {
        fprintf(stderr, "CRC mismatch. Payload may be corrupted.\n");
        free(compressed_payload);
        free(pixels);
        return 5;
    }


    decompressed_payload = lzw_decompress(compressed_payload, compressed_size_bits, &decompressed_payload_size);
    if (!decompressed_payload) {
        fprintf(stderr, "Decompression failed.\n");
        free(pixels);
        return 1;
    }

    output = fopen(output_name, "wb");
    if (!output) {
        fprintf(stderr, "Failed to open output file: %s\n", output_name);
        free(decompressed_payload);
        free(pixels);
        return 1;
    }
    fwrite(decompressed_payload, 1, decompressed_payload_size, output);
    fclose(output);

    free(decompressed_payload);
    free(pixels);
    return 0;
}




int read_png(const char* filename, Pixel** out_pixels, int* out_width, int* out_height) {
    if (!filename || !out_pixels || !out_width || !out_height) {
        fprintf(stderr, "Null pointer passed to read_png.\n");
        return 1;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Could not open file %s for reading.\n", filename);
        return 1;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        fprintf(stderr, "Could not allocate read struct.\n");
        return 1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        fprintf(stderr, "Could not allocate info struct.\n");
        return 1;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        fprintf(stderr, "Error during init_io.\n");
        return 1;
    }

    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
    png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    printf("Reading PNG with Width: %u, Height: %u\n", width, height);

    if (color_type != PNG_COLOR_TYPE_RGB || bit_depth != 8) {
        fprintf(stderr, "Unsupported image format. Require 24-bit RGB PNG.\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return 1;
    }

    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    if (!row_pointers) {
        fprintf(stderr, "Memory allocation failed for row pointers.\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return 1;
    }

    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr, info_ptr));
        if (!row_pointers[y]) {
            fprintf(stderr, "Memory allocation failed for row %d.\n", y);
            // Free previously allocated rows
            for (int j = 0; j < y; j++) {
                free(row_pointers[j]);
            }
            free(row_pointers);
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            fclose(fp);
            return 1;
        }
    }

    png_read_image(png_ptr, row_pointers);

    Pixel* pixels = (Pixel*) malloc(width * height * sizeof(Pixel));
    if (!pixels) {
        fprintf(stderr, "Memory allocation failed for pixel data.\n");
        for (int y = 0; y < height; y++) {
            free(row_pointers[y]);
        }
        free(row_pointers);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return 1;
    }

    for (int y = 0; y < height; y++) {
        png_bytep row = row_pointers[y];
        for (int x = 0; x < width; x++) {
            Pixel* pixel = &pixels[y * width + x];
            png_bytep px = &(row[x * 3]);
            pixel->blue = px[2];
            pixel->green = px[1];
            pixel->red = px[0];
        }
        free(row_pointers[y]);
    }
    free(row_pointers);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);

    *out_pixels = pixels;
    *out_width = (int)width;
    *out_height = (int)height;

    return 0;
}


int write_png(const char* filename, Pixel* pixels, int width, int height) {
    if (!filename || !pixels) {
        fprintf(stderr, "Null pointer passed to write_png.\n");
        return 1;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Could not open file %s for writing.\n", filename);
        return 1;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        fprintf(stderr, "Could not allocate write struct.\n");
        return 1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        fprintf(stderr, "Could not allocate info struct.\n");
        return 1;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        fprintf(stderr, "Error during writing initialization.\n");
        return 1;
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    if (!row_pointers) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        fprintf(stderr, "Memory allocation failed for row pointers.\n");
        return 1;
    }

    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr, info_ptr));
        if (!row_pointers[y]) {
            for (int j = 0; j < y; j++) {
                free(row_pointers[j]);
            }
            free(row_pointers);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            fclose(fp);
            fprintf(stderr, "Memory allocation failed for row %d.\n", y);
            return 1;
        }
        for (int x = 0; x < width; x++) {
            Pixel* pixel = &pixels[y * width + x];
            png_byte* row = &(row_pointers[y][x * 3]);
            row[0] = pixel->red;
            row[1] = pixel->green;
            row[2] = pixel->blue;
        }
    }

    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    for (int y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    printf("PNG written successfully to %s.\n", filename);
    return 0;
}




int embed_12bit_png(Pixel* pixels, int width, int height, const int* compressed_payload, int compressed_payload_size) {
    if (!pixels || !compressed_payload) {
        fprintf(stderr, "Null pointer passed to embed_12bit_png.\n");
        return 1;
    }

    size_t payload_in_bits, image_capacity, start_pixel, bit_position, pixel_index;
    unsigned char bit;
    int i, j;

    printf("compressedpayloadsizze %d\n", compressed_payload_size);
    payload_in_bits = compressed_payload_size * 12; // 12 bits per code
    image_capacity = (size_t)width * height;
    start_pixel = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS; // Adjust for signature and size field

    if (payload_in_bits + start_pixel > image_capacity) {
        fprintf(stderr, "Image does not have enough capacity for the payload. Available: %zu, Required: %zu\n", image_capacity, payload_in_bits + 32);
        return 1;
    }

    bit_position = 0;
    for (i = 0; i < compressed_payload_size; ++i) {
        for (j = 0; j < 12; ++j) {
            pixel_index = bit_position + start_pixel;
            if (pixel_index >= image_capacity) {
                fprintf(stderr, "Pixel index out of bounds. Index: %zu, Capacity: %zu\n", pixel_index, image_capacity);
                return 1;
            }
            bit = (compressed_payload[i] >> j) & 1;
            embed_bit(&pixels[pixel_index], bit);
            bit_position++;
        }
    }
    return 0;
}


void embed_bit(Pixel* pixel, size_t bit) {
    pixel->blue = (pixel->blue & 0xFE) | (bit & 1);
}



int extract_pixels_payload(const Pixel* pixels, int width, int height, int** out_compressed_payload, int* out_compressed_size) {
    if (!pixels || !out_compressed_payload || !out_compressed_size) {
        fprintf(stderr, "Null pointer passed to extract_pixels_payload.\n");
        return 1;
    }

    size_t start_pixel, total_pixels,  i, code_index, bit_index, pixel_index;
    unsigned int payload_size;
    unsigned char bit;

    start_pixel = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS; // Adjust for signature and size field
    payload_size = extract_size_png(pixels);
    *out_compressed_size = (int)(payload_size + 11) / 12;
    total_pixels = (size_t)width * height;


    if (payload_size + start_pixel > total_pixels * 8) {
        fprintf(stderr, "Payload size exceeds image capacity.\n");
        return 1;
    }

    *out_compressed_payload = (int*)malloc(*out_compressed_size * sizeof(int));
    if (*out_compressed_payload == NULL) {
        fprintf(stderr, "Failed to allocate memory for compressed payload extraction.\n");
        return 1;
    }

    memset(*out_compressed_payload, 0, *out_compressed_size * sizeof(int));
    for (i = 0; i < payload_size; ++i) {
        code_index = i / 12;
        bit_index = i % 12;
        pixel_index = i + start_pixel;

        if (pixel_index >= total_pixels) {
            fprintf(stderr, "Pixel index out of bounds. Index: %zu, Total Pixels: %zu\n", pixel_index, total_pixels);
            free(*out_compressed_payload);
            *out_compressed_payload = NULL;
            return 1;
        }

        bit = extract_bit(&pixels[pixel_index]);
        (*out_compressed_payload)[code_index] |= (bit << bit_index);
    }

    return 0;
}



void embed_size_png(Pixel* pixels, unsigned int size_in_bits) {
    if (!pixels) {
        fprintf(stderr, "Null pointer passed to embed_size_png.\n");
        return;
    }

    int start_index = SIGNATURE_SIZE_BITS;
    printf("Embedding payload size in bits: %u\n", size_in_bits);
    for (int i = 0; i < 32; ++i) {
        unsigned int bit = (size_in_bits >> (31 - i)) & 1;
        pixels[start_index + i].blue = (pixels[start_index + i].blue & 0xFE) | bit;
    }
}


unsigned int extract_bit(const Pixel* pixel) {
    if (!pixel) {
        fprintf(stderr, "Null pointer passed to extract_bit.\n");
        return 0;
    }

    unsigned int bit = pixel->blue & 1;
    return bit;
}

unsigned int extract_size_png(const Pixel* pixels) {
    if (!pixels) {
        fprintf(stderr, "Null pointer passed to extract_size_png.\n");
        return 0;
    }

    int start_index = SIGNATURE_SIZE_BITS; // Start index after the signature
    unsigned int size = 0;

    for (int i = 0; i < 32; ++i) {
        unsigned int bit = extract_bit(&pixels[start_index + i]);
        size |= (bit << (31 - i));
    }
    return size;
}
