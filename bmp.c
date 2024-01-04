#include "files.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lzw.h"
#include "bmp.h"
#include "checksuma.h"
#define INT_MAX  2147483647


void read_bitmap_headers(FILE* input_file, BITMAPFILEHEADER* bfh, BITMAPINFOHEADER* bih) {
    fread(bfh, sizeof(BITMAPFILEHEADER), 1, input_file);
    fread(bih, sizeof(BITMAPINFOHEADER), 1, input_file);
}

int calculate_padding(BITMAPINFOHEADER bih) {
    return (4 - (bih.width * sizeof(Pixel)) % 4) % 4;
}

int embed_to_bmp(const char* image_filename, const char* output_image_filename, const int* compressed_payload, int compressed_size, const char* payloadFilename) {
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    FILE* input_file;
    int padding, pixel_data_size, num_pixels, payload_bits, available_bits, crc_bits;
    uint32_t crc;
    Pixel* pixels;

    input_file = fopen(image_filename, "rb");
    if (!input_file) {
        fprintf(stderr, "Failed to open BMP file.\n");
        return 1;
    }

    read_bitmap_headers(input_file, &bfh, &bih);

    padding = calculate_padding(bih);
    pixel_data_size = bih.width * abs(bih.height) * sizeof(Pixel) + padding * abs(bih.height);

    pixels = read_pixel_data(input_file, bfh, bih, &pixel_data_size);
    if (!pixels) {
        fprintf(stderr, "Failed to read BMP file: %s\n", image_filename);
        fclose(input_file);
        return 1;
    }

    num_pixels = bih.width * abs(bih.height);
    payload_bits = compressed_size * 12; /* Payload size in bits */
    available_bits = num_pixels - SIZE_FIELD_BITS - SIGNATURE_SIZE_BITS - CRC_SIZE_BITS; /* Available bits for payload, minus 32 for the size */

    if (payload_bits > available_bits) {
        fprintf(stderr, "Not enough space in the image for the payload. Payload is %d bits and available is %d bits\n", payload_bits, available_bits);
        free(pixels);
        return 3;
    }

    embed_signature(pixels);
    embed_size(pixels, payload_bits);
    embed_12bit_payload(pixels, num_pixels, compressed_payload, compressed_size);

    crc = calculate_crc(compressed_payload, payload_bits);

    embed_crc(pixels, bih.width, abs(bih.height), crc, payload_bits);

    if (!save_image(output_image_filename, bfh, bih, (unsigned char *) pixels, pixel_data_size)) {
        fprintf(stderr, "Failed to create output image with embedded payload.\n");
        free(pixels);
        return 1;
    }

    free(pixels);
    return 0;
}


void read_bitmap(const char* filename, BITMAPFILEHEADER* bfh, BITMAPINFOHEADER* bih, Pixel** pixels, int* pixel_data_size) {
    FILE* input_file;

    input_file = fopen(filename, "rb");
    if (!input_file) {
        fprintf(stderr, "Failed to open BMP file.\n");
        exit(1);
    }

    read_bitmap_headers(input_file, bfh, bih);
    *pixels = read_pixel_data(input_file, *bfh, *bih, pixel_data_size);
    fclose(input_file);
}

void save_decompressed_payload(const unsigned char* decompressed_payload, int size, const char* filename) {
    FILE* output_file;

    output_file = fopen(filename, "wb");
    if (!output_file) {
        fprintf(stderr, "Failed to open output file for decompressed payload.\n");
        free((unsigned char*)decompressed_payload); /* Casting to unsigned char* is more appropriate */
        exit(1);
    }

    fwrite(decompressed_payload, 1, size, output_file);
    fclose(output_file);
}



int extract_payload(const char* input_name, const char* output_name) {
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    Pixel* pixels;
    int pixel_data_size;
    int compressed_payload_size;
    int* compressed_payload;
    unsigned long extracted_crc, calculated_crc;
    int decompressed_payload_size;
    unsigned char* decompressed_payload;

    read_bitmap(input_name, &bfh, &bih, &pixels, &pixel_data_size);
    if (!pixels) {
        fprintf(stderr, "Failed to read pixel data.\n");
        return 1;
    }

    /* Check if the signature matches */
    if (!check_signature(pixels)) {
        fprintf(stderr, "Signature mismatch or not found.\n");
        free(pixels);
        return 4;
    }

    /* Extract compressed payload from image */
    compressed_payload = extract_12bit_payload(pixels, (pixel_data_size / sizeof(Pixel)), &compressed_payload_size);
    if (!compressed_payload) {
        fprintf(stderr, "Failed to extract payload.\n");
        free(pixels);
        return 1;
    }

    /* Extract and calculate CRC */
    extracted_crc = extract_crc(pixels, bih.width, abs(bih.height), compressed_payload_size * 12);
    calculated_crc = calculate_crc(compressed_payload, compressed_payload_size * 12);

    /* Compare the extracted CRC with the calculated CRC */
    if (extracted_crc != calculated_crc) {
        fprintf(stderr, "CRC mismatch. Payload may be corrupted.\n");
        free(compressed_payload);
        free(pixels);
        return 5;
    }
    free(pixels);

    /* Decompress the extracted payload */
    decompressed_payload = lzw_decompress(compressed_payload, compressed_payload_size, &decompressed_payload_size);
    free(compressed_payload);
    if (!decompressed_payload) {
        fprintf(stderr, "Failed to decompress payload.\n");
        return 1;
    }

    /* Save decompressed payload to file */
    save_decompressed_payload(decompressed_payload, decompressed_payload_size, output_name);
    free(decompressed_payload);

    return 0;
}



unsigned int extract_size_from_pixels(const Pixel* pixels, int num_pixels) {
    int start_index, i;
    unsigned int size, bit;

    start_index = SIGNATURE_SIZE_BITS;

    if (num_pixels < start_index + 32) {
        fprintf(stderr, "Not enough pixels to extract payload size.\n");
        return 0;
    }

    size = 0;
    for (i = 0; i < 32; ++i) {
        bit = pixels[start_index + i].blue & 1;
        size |= (bit << i);
    }

    printf("Final extracted size: %u\n", size);
    return size;
}



Pixel* read_pixel_data(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, int* pixel_data_size) {
    int row_size, padding, i;
    long total_size;
    Pixel* pixel_data;

    /* Calculate padding safely */
    row_size = bih.width * sizeof(Pixel);
    padding = (4 - (row_size % 4)) % 4;
    total_size = (long)row_size + padding;
    total_size *= abs(bih.height);

    /* Check for overflow or too large size */
    if (total_size > INT_MAX) {
        fprintf(stderr, "Pixel data size too large.\n");
        fclose(file);
        return NULL;
    }

    *pixel_data_size = (int)total_size;
    pixel_data = (Pixel*)malloc(*pixel_data_size);
    if (!pixel_data) {
        fprintf(stderr, "Memory allocation failed for pixel data.\n");
        fclose(file);
        return NULL;
    }

    /* Set the file position to the beginning of pixel data */
    fseek(file, bfh.offset, SEEK_SET);

    for (i = 0; i < abs(bih.height); ++i) {
        if (fread(pixel_data + (bih.width * i), sizeof(Pixel), bih.width, file) != bih.width) {
            fprintf(stderr, "Failed to read pixel data.\n");
            free(pixel_data);  /* Free allocated memory in case of read error */
            fclose(file);
            return NULL;
        }
        fseek(file, padding, SEEK_CUR);
    }

    *pixel_data_size = bih.width * abs(bih.height) * sizeof(Pixel);
    return pixel_data;
}

void set_lsb(unsigned char* byte, int bitValue) {
    /* Ensure the bitValue is either 0 or 1 */
    bitValue = (bitValue != 0) ? 1 : 0;
    *byte = (*byte & 0xFE) | bitValue;
}

void embed_size(Pixel* pixels, unsigned int size) {
    int start, end, i;
    unsigned int bit;

    printf("Embedding size %u\n", size);

    start = SIGNATURE_SIZE_BITS; /* Start embedding after the signature */
    end = start + 32; /* The size field is 32 bits */

    for (i = start; i < end; ++i) {
        bit = (size >> (i - start)) & 1; /* Adjust bit index by subtracting start */
        set_lsb(&pixels[i].blue, bit);
    }


}

/*
int getBit(const int* data, int size, int position) {
    int byteIndex = position / 32;
    int bitIndex = position % 32;

    if (byteIndex >= size) {
        fprintf(stderr, "Error: Bit position out of bounds.\n");
        return -1; // Indicate an error
    }

    return (data[byteIndex] >> bitIndex) & 1;
}
 */

void embed_12bit_payload(Pixel* pixels, int num_pixels, const int* compressed_payload, int compressed_size) {
    int total_bits, bit_position, start_pixel, i, payload_index, bit_index_in_payload, bit;

    total_bits = compressed_size * 12; /* 12 bits per code */

    bit_position = 0;
    start_pixel = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS;

    for (i = start_pixel; bit_position < total_bits; ++i) {
        if (i >= num_pixels) {
            fprintf(stderr, "Error: Reached the end of the pixel array. i: %d, num_pixels: %d\n", i, num_pixels);
            break;
        }

        payload_index = bit_position / 12;
        bit_index_in_payload = bit_position % 12;
        bit = (compressed_payload[payload_index] >> bit_index_in_payload) & 1;

        set_lsb(&pixels[i].blue, bit);
        bit_position++;

        if (bit_position >= total_bits) {
            printf("Successfully embedded all bits. Last bit position: %d\n", bit_position);
            break;
        }
    }

    /* Check if all bits were embedded */
    if (bit_position != total_bits) {
        fprintf(stderr, "Error: Not all payload bits were embedded. Embedded: %d, Expected: %d\n", bit_position, total_bits);
    }
}

int* extract_12bit_payload(const Pixel* pixels, int num_pixels, int* compressed_payload_size) {
    int start_index, payload_index, bit_index_in_payload, bit, bit_position, i;
    unsigned int payload_bit_size;
    int* payload;

    start_index = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS; /* This should be 80 */

    payload_bit_size = extract_size_from_pixels(pixels, num_pixels);

    *compressed_payload_size = (payload_bit_size + 11) / 12; /* Calculate the number of 12-bit codes */

    payload = (int*)malloc(*compressed_payload_size * sizeof(int));
    if (!payload) {
        fprintf(stderr, "Memory allocation failed for payload extraction.\n");
        return NULL;
    }

    memset(payload, 0, *compressed_payload_size * sizeof(int));

    bit_position = 0;
    for (i = start_index; i < payload_bit_size; ++i) {
        payload_index = bit_position / 12;
        bit_index_in_payload = bit_position % 12;

        bit = pixels[i].blue & 1;
        payload[payload_index] |= (bit << bit_index_in_payload);

        bit_position++;
        if (bit_position >= payload_bit_size) {
            break;
        }
    }

    return payload;
}
