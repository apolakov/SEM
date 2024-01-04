#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "files.h"
#include "bmp.h"


int determine_type(const char *filename) {
    printf("determining");
    FILE *file = fopen(filename, "rb");
    int fileTypeCheck;
    if (!file) {
        perror("File opening failed");
        return 1; // Return 1 to indicate failure
    }

    fileTypeCheck = 1; // Default to 1 (failure)

    if (is_png(file) && is_24bit_png(file)) {
        fileTypeCheck = 3; // It's a 24-bit PNG file
    } else {
        rewind(file); // Necessary for BMP check
        if (is_bmp(file) && is_24bit_bmp(file)) {
            fileTypeCheck = 2; // It's a 24-bit BMP file
        }
    }

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

    rewind(file); /* Ensure the file is read from the beginning */

    if (fread(buffer, 1, 2, file) == 2) {
        return memcmp(buffer, bmp_signature, 2) == 0;
    }
    return 0;
}

int is_24bit_bmp(FILE *file) {
    unsigned long dib_header_size;
    unsigned short bits_per_pixel;

    rewind(file);

    /* Skip the BMP header which is 14 bytes */
    if (fseek(file, 14, SEEK_SET) != 0) {
        return 0;
    }

    /* Read the size of the DIB header */
    if (fread(&dib_header_size, sizeof(dib_header_size), 1, file) != 1) {
        return 0;
    }

    /* Skip to the bits_per_pixel field */
    if (fseek(file, 10, SEEK_CUR) != 0) {
        return 0;
    }

    /* Read the bits_per_pixel */
    if (fread(&bits_per_pixel, sizeof(bits_per_pixel), 1, file) != 1) {
        return 0;
    }

    /* The BMP is 24-bit if bits_per_pixel is 24 */
    return bits_per_pixel == 24;
}

int save_image(const char* filename, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, unsigned char* pixelData, int pixelDataSize) {
    FILE* file;
    size_t bytesWritten;
    int padding, i;
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
        if (bytesWritten != bih.width * 3) {
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

int write_compressed(const char* filename, const unsigned long* compressedPayload, int compressedSize) {
    FILE* file;
    unsigned long totalBytes;
    unsigned char* packedData;
    int bitPosition, i, j, code;
    size_t bytesWritten;
    unsigned long byteIndex, bitIndex;

    file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing compressed payload.\n");
        return 1;
    }

    totalBytes = (compressedSize * 12 + 7) / 8;
    packedData = (unsigned char*)malloc(totalBytes);
    if (!packedData) {
        fprintf(stderr, "Memory allocation failed for packed data.\n");
        fclose(file);
        return 1;
    }

    memset(packedData, 0, totalBytes);
    bitPosition = 0;
    for (i = 0; i < compressedSize; i++) {
        code = compressedPayload[i];
        for (j = 0; j < 12; j++) {
            byteIndex = (bitPosition / 8);
            bitIndex = bitPosition % 8;
            packedData[byteIndex] |= ((code >> j) & 1) << bitIndex;
            bitPosition++;
        }
    }

    bytesWritten = fwrite(&compressedSize, sizeof(compressedSize), 1, file);
    bytesWritten += fwrite(packedData, 1, totalBytes, file);

    free(packedData);
    fclose(file);

    return (bytesWritten == totalBytes + 1) ? 0 : 1;
}


