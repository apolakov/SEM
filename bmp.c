#include "files.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lzw.h"
#include "bmp.h"
#include "checksuma.h"
#include <limits.h>
/*#define INT_MAX  2147483647*/


void read_bitmap_headers(FILE* input_file, BITMAPFILEHEADER* bfh, BITMAPINFOHEADER* bih) {
    long filePos = ftell(input_file);
    printf("Initial File Position: %ld\n", filePos);

    if (fread(&bfh->type, 2, 1, input_file) != 1) return; // 2 bytes
    if (fread(&bfh->size, 4, 1, input_file) != 1) return; // 4 bytes
    fread(&bfh->reserved1, sizeof(bfh->reserved1), 1, input_file); // Read as 2 bytes
    fread(&bfh->reserved2, sizeof(bfh->reserved2), 1, input_file); // Read as 2 bytes
    fread(&bfh->offset, sizeof(bfh->offset), 1, input_file); // Read as 4 bytes

    if (fread(&bih->size, 4, 1, input_file) != 1) return; // 4 bytes

    unsigned char widthBytes[4];
    unsigned char heightBytes[4];
    if (fread(widthBytes, 4, 1, input_file) != 1) return; // 4 bytes
    if (fread(heightBytes, 4, 1, input_file) != 1) return; // 4 bytes

    filePos = ftell(input_file);
    printf("File Position after reading Width and Height: %ld\n", filePos);

    printf("\n");
    printf("Raw Width Bytes: ");
    for (int i = 0; i < 4; i++) printf("%02X ", widthBytes[i]);
    printf("\n");

    printf("Raw Height Bytes: ");
    for (int i = 0; i < 4; i++) printf("%02X ", heightBytes[i]);
    printf("\n");

    bih->width = (long)(widthBytes[0] | widthBytes[1] << 8 | widthBytes[2] << 16 | widthBytes[3] << 24);
    bih->height = (long)(heightBytes[0] | heightBytes[1] << 8 | heightBytes[2] << 16 | heightBytes[3] << 24);

    // Continue reading the rest of bih
    fread(&bih->planes, sizeof(bih->planes), 1, input_file); // Read as 2 bytes
    fread(&bih->bits, sizeof(bih->bits), 1, input_file); // Read as 2 bytes
    fread(&bih->compression, sizeof(bih->compression), 1, input_file); // Read as 4 bytes
    fread(&bih->imagesize, sizeof(bih->imagesize), 1, input_file); // Read as 4 bytes
    fread(&bih->xresolution, sizeof(bih->xresolution), 1, input_file); // Read as 4 bytes
    fread(&bih->yresolution, sizeof(bih->yresolution), 1, input_file); // Read as 4 bytes
    fread(&bih->ncolours, sizeof(bih->ncolours), 1, input_file); // Read as 4 bytes
    fread(&bih->importantcolours, sizeof(bih->importantcolours), 1, input_file); // Read as 4 bytes

    printf("Read Width: %ld, Height: %ld\n", bih->width, bih->height);
}

int calculate_padding(BITMAPINFOHEADER bih) {
    int width = (int)bih.width;

    return (4 - (width * (int)sizeof(Pixel)) % 4) % 4;
}

int embed_to_bmp(const char* image_filename, const char* output_image_filename, const int* compressed_payload, int compressed_size) {
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    FILE* input_file;
    int padding, num_pixels, payload_bits, available_bits;
    long num_pixels_long;
    size_t pixel_data_size;
    uint32_t crc;
    Pixel* pixels;

    input_file = fopen(image_filename, "rb");
    if (!input_file) {
        fprintf(stderr, "Failed to open BMP file.\n");
        return 1;
    }

    read_bitmap_headers(input_file, &bfh, &bih);

    padding = calculate_padding(bih);
    pixel_data_size = (size_t)bih.width * (size_t)labs(bih.height) * sizeof(Pixel) + (size_t)padding * labs(bih.height);

    pixels = read_pixel_data(input_file, bfh, bih, &pixel_data_size);
    if (!pixels) {
        fprintf(stderr, "Failed to read BMP file: %s\n", image_filename);
        fclose(input_file);
        return 1;
    }

    num_pixels_long = (long)bih.width * (long)labs(bih.height);

    // Ensure that num_pixels_long does not exceed the limit of int
    if (num_pixels_long > INT_MAX || num_pixels_long < 0) {
        fprintf(stderr, "Error: Number of pixels exceeds the limit or is negative.\n");
        fclose(input_file);  // Close the file before returning
        free(pixels);
        return 3;
    }
    num_pixels = (int)num_pixels_long;

    payload_bits = compressed_size * 12; /* Payload size in bits */
    available_bits = num_pixels * 8 - (SIZE_FIELD_BITS + SIGNATURE_SIZE_BITS + CRC_SIZE_BITS);

    if (payload_bits > available_bits) {
        fprintf(stderr, "Not enough space in the image for the payload. Payload is %d bits and available is %d bits\n", payload_bits, available_bits);
        free(pixels);
        return 3;
    }

    embed_signature(pixels);
    embed_size(pixels, payload_bits);
    embed_12bit_payload(pixels, num_pixels, compressed_payload, compressed_size);

    crc = calculate_crc(compressed_payload, payload_bits);

    embed_crc(pixels, bih.width, labs(bih.height), crc, payload_bits);

    printf("i am trying to save picture\n");
    if (!save_image(output_image_filename, bfh, bih, (unsigned char *) pixels)) {
        fprintf(stderr, "Failed to create output image with embedded payload.\n");
        free(pixels);
        return 1;
    }

    free(pixels);
    return 0;
}


void read_bitmap(const char* filename, BITMAPFILEHEADER* bfh, BITMAPINFOHEADER* bih, Pixel** pixels, int* pixel_data_size) {
    FILE* input_file;
    size_t temp_pixel_data_size;

    input_file = fopen(filename, "rb");
    if (!input_file) {
        fprintf(stderr, "Failed to open BMP file.\n");
        exit(1);
    }

    read_bitmap_headers(input_file, bfh, bih);
    *pixels = read_pixel_data(input_file, *bfh, *bih, &temp_pixel_data_size);
    *pixel_data_size = (int)temp_pixel_data_size;
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
    unsigned long tmp_crc_size;


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
    compressed_payload = extract_12bit_payload(pixels, (int)(pixel_data_size / sizeof(Pixel)), &compressed_payload_size);
    if (!compressed_payload) {
        fprintf(stderr, "Failed to extract payload.\n");
        free(pixels);
        return 1;
    }


    /* Extract and calculate CRC */
    tmp_crc_size = (unsigned long)compressed_payload_size * 12;
    extracted_crc = (tmp_crc_size <= UINT_MAX) ? extract_crc(pixels, bih.width, labs(bih.height), (int)tmp_crc_size) : 0;
    calculated_crc = (tmp_crc_size <= UINT_MAX) ? calculate_crc(compressed_payload, (int)tmp_crc_size) : 0;

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



Pixel* read_pixel_data(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, size_t* pixel_data_size){
    int row_size, padding, i;
    long total_size;
    Pixel* pixel_data;

    /* Calculate padding safely */
    row_size = (int)(bih.width * sizeof(Pixel));
    padding = (4 - (row_size % 4)) % 4;
    total_size = (long)row_size + padding;
    total_size *= labs(bih.height);

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
    fseek(file, (long)bfh.offset, SEEK_SET);

    for (i = 0; i < labs(bih.height); ++i) {
        if ((int)fread(pixel_data + (bih.width * i), sizeof(Pixel), bih.width, file) != (int)bih.width) {
            fprintf(stderr, "Failed to read pixel data.\n");
            free(pixel_data);  /* Free allocated memory in case of read error */
            fclose(file);
            return NULL;
        }
        fseek(file, padding, SEEK_CUR);
    }

    *pixel_data_size = (size_t)bih.width * (size_t)labs(bih.height) * sizeof(Pixel);
    return pixel_data;
}

void set_lsb(unsigned char* byte, int bitValue) {
    /* Ensure the bitValue is either 0 or 1 */
    bitValue = (bitValue != 0) ? 1 : 0;
    *byte = (*byte & 0xFE) | bitValue;
}

void embed_size(Pixel* pixels, unsigned int size) {
    int start = SIGNATURE_SIZE_BITS;
    int end = start + 32; // The size field is 32 bits
    int i;
    unsigned int bit;

    printf("Embedding size %u\n", size);

    for (i = start; i < end; ++i) {
        bit = (size >> (i - start)) & 1;
        set_lsb(&pixels[i].blue, (int)bit);
    }
}


void embed_12bit_payload(Pixel* pixels, int num_pixels, const int* compressed_payload, int compressed_size) {
    long total_bits = (long)compressed_size * 12; // Use long to avoid overflow
    long bit_position = 0;
    int start_pixel = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS;
    int i, payload_index, bit_index_in_payload, bit;

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
            printf("Successfully embedded all bits. Last bit position: %ld\n", bit_position);
            break;
        }
    }

    if (bit_position != total_bits) {
        fprintf(stderr, "Error: Not all payload bits were embedded. Embedded: %ld, Expected: %ld\n", bit_position, total_bits);
    }
}


int* extract_12bit_payload(const Pixel* pixels, int num_pixels, int* compressed_payload_size) {
    int start_index, payload_index, bit_index_in_payload, bit, bit_position, i;
    unsigned int payload_bit_size;
    int* payload;

    start_index = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS; /* This should be 80 */

    payload_bit_size = extract_size_from_pixels(pixels, num_pixels);

    *compressed_payload_size = (int)(payload_bit_size + 11) / 12; /* Calculate the number of 12-bit codes */

    payload = (int*)malloc(*compressed_payload_size * sizeof(int));
    if (!payload) {
        fprintf(stderr, "Memory allocation failed for payload extraction.\n");
        return NULL;
    }

    memset(payload, 0, *compressed_payload_size * sizeof(int));

    bit_position = 0;
    for (i = start_index; i < (int)payload_bit_size; ++i) {
        payload_index = bit_position / 12;
        bit_index_in_payload = bit_position % 12;

        bit = pixels[i].blue & 1;
        payload[payload_index] |= (bit << bit_index_in_payload);

        bit_position++;
        if (bit_position >= (int)payload_bit_size) {
            break;
        }
    }

    return payload;
}