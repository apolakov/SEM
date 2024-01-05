#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "files.h"
#include "bmp.h"


int determine_type(const char *filename) {

    FILE *file = fopen(filename, "rb");
    int fileTypeCheck;
    if (!file) {
        perror("File opening failed");
        return 1;
    }

    fileTypeCheck = 1;

    if (is_png(file) && is_24bit_png(file)) {
        printf("returning 3 as a type\n");
        fileTypeCheck = 3;
    } else {
        rewind(file);
        if (is_bmp(file) && is_24bit_bmp(file)) {
            printf("returning 3 as a type\n");
            fileTypeCheck = 2;
        }
    }

    printf("returning 1 as a type\n");
    fclose(file);
    return fileTypeCheck;
}


int is_24bit_png(FILE *file) {
    char chunk_type[5];
    unsigned char bit_depth;
    unsigned char color_type;
    unsigned long chunk_length;

    rewind(file);

    /* Skipping PNG signature */
    if (fseek(file, 8, SEEK_SET) != 0) {
        return 0;
    }

    /* Read IHDR chunk length and type */
    if (fread(&chunk_length, sizeof(chunk_length), 1, file) != 1) {
        return 0;
    }

    if (fread(chunk_type, 1, 4, file) != 4) {
        return 0;
    }
    chunk_type[4] = '\0'; /* Null-terminate the string */

    /* Check if it is IHDR chunk */
    if (strcmp(chunk_type, "IHDR") != 0) {
        return 0;
    }

    /* Skip width and height */
    if (fseek(file, 8, SEEK_CUR) != 0) {
        return 0;
    }

    /* Read bit depth and color type */
    if (fread(&bit_depth, sizeof(bit_depth), 1, file) != 1) {
        return 0;
    }

    if (fread(&color_type, sizeof(color_type), 1, file) != 1) {
        return 0;
    }

    /* Check for 24-bit RGB: color type 2 and bit depth 8 */
    return color_type == 2 && bit_depth == 8;
}




int is_png(FILE *file) {
    unsigned char png_signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    unsigned char buffer[8];

    rewind(file); /* Ensure the file is read from the beginning */

    if (fread(buffer, 1, 8, file) == 8) {
        return memcmp(buffer, png_signature, 8) == 0;
    }
    return 0;
}

int is_bmp(FILE *file) {
    unsigned char bmp_signature[2] = {0x42, 0x4D};
    unsigned char buffer[2];

    rewind(file);

    if (fread(buffer, 1, 2, file) == 2) {
        printf("BMP Signature: %02X %02X\n", buffer[0], buffer[1]);
        return memcmp(buffer, bmp_signature, 2) == 0;
    } else {
        printf("Failed to read BMP signature.\n");
    }
    return 0;
}

int is_24bit_bmp(FILE *file) {
    unsigned long dib_header_size;
    unsigned short bits_per_pixel;

    rewind(file);

    if (fseek(file, 14, SEEK_SET) != 0) {
        printf("Failed to seek to DIB header.\n");
        return 0;
    }

    if (fread(&dib_header_size, sizeof(dib_header_size), 1, file) != 1) {
        printf("Failed to read DIB header size.\n");
        return 0;
    }

    if (fseek(file, 10, SEEK_CUR) != 0) {
        printf("Failed to seek to bits per pixel field.\n");
        return 0;
    }

    if (fread(&bits_per_pixel, sizeof(bits_per_pixel), 1, file) != 1) {
        printf("Failed to read bits per pixel.\n");
        return 0;
    }

    printf("Bits per pixel: %d\n", bits_per_pixel);

    return bits_per_pixel == 24;
}

int save_image(const char* filename, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, unsigned char* pixelData) {
    FILE* file;
    size_t bytesWritten;
    size_t padding;
    int i;
    unsigned char pad[3] = {0};

    file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Unable to open output file: %s\n", filename);
        return 0;
    }

    /* Write the BMP file header */
    bytesWritten = fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, file);
    if (bytesWritten != 1) {
        fprintf(stderr, "Failed to write BMP file header.\n");
        fclose(file);
        return 0;
    }

    /* Write the BMP info header */
    bytesWritten = fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, file);
    if (bytesWritten != 1) {
        fprintf(stderr, "Failed to write BMP info header.\n");
        fclose(file);
        return 0;
    }

    /* Compute padding for each row */
    padding = (4 - (bih.width * 3) % 4) % 4;

    for (i = 0; i < abs(bih.height); i++) {
        unsigned char* rowData = pixelData + (bih.width * 3) * i;

        /* Write one row of pixels */
        bytesWritten = fwrite(rowData, 1, bih.width * 3, file);
        if ((int)bytesWritten != bih.width * 3) {
            fprintf(stderr, "Failed to write pixel data for row %d.\n", i);
            fclose(file);
            return 0;
        }

        /* Write padding */
        bytesWritten = fwrite(pad, 1, padding, file);
        if (bytesWritten != padding) {
            fprintf(stderr, "Failed to write padding for row %d.\n", i);
            fclose(file);
            return 0;
        }
    }

    fclose(file);
    return 1;
}
