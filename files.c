#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "files.h"
#include "bmp.h"
#include <stdint.h>


int determine_type(const char *filename) {
    FILE *file = fopen(filename, "rb"); // Open the file in binary mode
    if (file == NULL) {
        printf("Cannot open file.\\n");
        return 1;
    }

    unsigned char header[54]; // To store the header information
    size_t bytesRead = fread(header, sizeof(unsigned char), 54, file); // Read the first 54 bytes of the file header
    if (bytesRead < 54) {
        printf("File too small to be a valid image.\\n");
        fclose(file);
        return 1;
    }

    // Print the first few bytes of the file for debugging
    printf("Header: ");
    for (int i = 0; i < 8; i++) {
        printf("%02x ", header[i]);
    }
    printf("\\n");

    // Check for BMP file
    if (header[0] == 'B' && header[1] == 'M') {
        int bitDepth = header[28]; // Bit depth is stored at offset 28
        fclose(file); // Close the file before returning
        if (bitDepth == 24) {
            printf("The file is a 24-bit BMP file.\\n");
            return 2;
        } else {
            printf("The file is a BMP file, but not 24-bit.\\n");
            return 1;
        }
    }
        // Simplified PNG signature check
    else if (header[0] == 0x89 && header[1] == 'P' && header[2] == 'N' && header[3] == 'G' &&
             header[4] == '\r' && header[5] == '\n' && header[6] == 0x1A && header[7] == '\n') {
        printf("PNG signature matched.\\n");
        unsigned char bitDepth = header[24]; // Correct offset for bit depth
        unsigned char colorType = header[25]; // Correct offset for color type
        fclose(file); // Close the file before returning
        if (colorType == 2 && bitDepth == 8) { // Truecolor PNG with 8-bit per channel
            printf("The file is a 24-bit PNG file.\\n");
            return 3; // Return 0 for successful identification
        } else {
            printf("The file is a PNG file, but not 24-bit.\\n");
            return 1;
        }
    }
    else {
        fclose(file); // Close the file before returning
        printf("The file is neither a 24-bit BMP file nor a PNG file.\\n");
        return 1;
    }
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

    for (i = 0; i < labs(bih.height); i++) {
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
    printf("Image is saved\n");
    return 1;
}