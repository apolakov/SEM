#include "files.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
//#include "function.h"
#include <stdlib.h>
#include "lzw.h"
#include "bmp.h"
#include "checksuma.h"


void readBitmapHeaders(FILE* inputFile, BITMAPFILEHEADER* bfh, BITMAPINFOHEADER* bih) {
    fread(bfh, sizeof(BITMAPFILEHEADER), 1, inputFile);
    fread(bih, sizeof(BITMAPINFOHEADER), 1, inputFile);
}

int calculatePadding(BITMAPINFOHEADER bih) {
    return (4 - (bih.width * sizeof(Pixel)) % 4) % 4;
}

int embedPayloadInImage(const char* imageFilename, const char* outputImageFilename, const int* compressedPayload, int compressedSize, const char* payloadFilename) {
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    FILE* inputFile = fopen(imageFilename, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open BMP file.\n");
        return 1;
    }

    readBitmapHeaders(inputFile, &bfh, &bih);

    int padding = calculatePadding(bih);
    int pixelDataSize = bih.width * abs(bih.height) * sizeof(Pixel) + padding * abs(bih.height);

    Pixel* pixels = readPixelData(inputFile, bfh, bih, &pixelDataSize);
    fclose(inputFile);
    if (!pixels) {
        fprintf(stderr, "Failed to read pixel data.\n");
        return 1;
    }

    printf("I got here\n");
    int numPixels = bih.width * abs(bih.height);
    int payloadBits = compressedSize * 12; // Payload size in bits
    int availableBits = numPixels - SIZE_FIELD_BITS - SIGNATURE_SIZE_BITS; // Available bits for payload, minus 32 for the size

    if (payloadBits > availableBits) {
        fprintf(stderr, "Not enough space in the image for the payload. Payload is %d and available is %d\n", payloadBits, availableBits);
        free(pixels);
        return 1;
    }

    embedSignature(pixels);


    embedSize(pixels, payloadBits);

    embed12BitPayload(pixels, numPixels, compressedPayload, compressedSize);

    if (!saveImage(outputImageFilename, bfh, bih, (unsigned char*)pixels, pixelDataSize)) {
        fprintf(stderr, "Failed to create output image with embedded payload.\n");
        free(pixels);
        return 1;
    }

    free(pixels);
    return 0;
}


void readBitmapHeadersAndPixelData(const char* filename, BITMAPFILEHEADER* bfh, BITMAPINFOHEADER* bih, Pixel** pixels, int* pixelDataSize) {
    FILE* inputFile = fopen(filename, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open BMP file.\n");
        exit(1);
    }
    readBitmapHeaders(inputFile, bfh, bih);
    *pixels = readPixelData(inputFile, *bfh, *bih, pixelDataSize);
    fclose(inputFile);
}

void saveDecompressedPayload(const unsigned char* decompressedPayload, int size, const char* filename) {
    FILE* outputFile = fopen(filename, "wb");
    if (!outputFile) {
        fprintf(stderr, "Failed to open output file for decompressed payload.\n");
        free((void*)decompressedPayload);
        exit(1);
    }
    fwrite(decompressedPayload, 1, size, outputFile);
    fclose(outputFile);
}



int extractAndDecompressPayload(const char* inputImageFilename, const char* outputPayloadBaseFilename) {
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    Pixel* pixels;
    int pixelDataSize;

    readBitmapHeadersAndPixelData(inputImageFilename, &bfh, &bih, &pixels, &pixelDataSize);
    if (!pixels) {
        fprintf(stderr, "Failed to read pixel data.\n");
        return 1;
    }

    // Check if the signature matches
    if (!extractAndCheckSignature(pixels)) {
        fprintf(stderr, "Signature mismatch or not found.\n");
        free(pixels);
        return 1;
    }

    int compressedPayloadSize;
    int* compressedPayload = extract12BitPayload(pixels, (pixelDataSize / sizeof(Pixel)), &compressedPayloadSize);
    free(pixels);
    if (!compressedPayload) {
        fprintf(stderr, "Failed to extract payload.\n");
        return 1;
    }

    int decompressedPayloadSize;
    unsigned char* decompressedPayload = lzwDecompress(compressedPayload, compressedPayloadSize, &decompressedPayloadSize);
    free(compressedPayload);
    if (!decompressedPayload) {
        fprintf(stderr, "Failed to decompress payload.\n");
        return 1;
    }

    saveDecompressedPayload(decompressedPayload, decompressedPayloadSize, outputPayloadBaseFilename);
    free(decompressedPayload);

    return 0;
}



unsigned int extractSizeFromPixelData(const Pixel* pixels, int numPixels) {
    int startIndex = SIGNATURE_SIZE_BITS ; // Start index after the signature

    if (numPixels < startIndex + 32) {
        fprintf(stderr, "Not enough pixels to extract payload size.\n");
        return 0;
    }

    unsigned int size = 0;
    for (int i = 0; i < 32; ++i) {
        unsigned int bit = pixels[startIndex + i].blue & 1; // Adjust index by adding startIndex
        size |= (bit << i);
        // printf("Extracting bit %d from pixel %d: %d, Cumulative size: %u\n", bit, startIndex + i, pixels[startIndex + i].blue, size);
    }

    printf("Final extracted size: %u\n", size);
    printf("SIZE2 %d\n",size);
    return size;
}


Pixel* readPixelData(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, int* pixelDataSize) {
    // Calculate the padding for each row (BMP rows are aligned to 4-byte boundaries)
    int padding = (4 - (bih.width * sizeof(Pixel)) % 4) % 4;

    // Calculate the size of the pixel data (including padding)
    *pixelDataSize = bih.width * abs(bih.height) * sizeof(Pixel) + padding * abs(bih.height);

    // Allocate memory for pixel data
    Pixel* pixelData = (Pixel*)malloc(*pixelDataSize);
    if (!pixelData) {
        fprintf(stderr, "Memory allocation failed for pixel data.\n");
        fclose(file);
        return NULL;
    }

    // Set the file position to the beginning of pixel data
    fseek(file, bfh.offset, SEEK_SET);

    // Read pixel data row by row
    for (int i = 0; i < abs(bih.height); ++i) {
        // Read one row of pixel data
        if (fread(pixelData + (bih.width * i), sizeof(Pixel), bih.width, file) != bih.width) {
            fprintf(stderr, "Failed to read pixel data.\n");
            free(pixelData);  // Free allocated memory in case of read error
            fclose(file);
            return NULL;
        }
        // Skip over padding
        fseek(file, padding, SEEK_CUR);
    }

    *pixelDataSize = bih.width * abs(bih.height) * sizeof(Pixel);
    return pixelData;
}



// Embedding the size
void embedSize(Pixel* pixels, unsigned int size) {
    printf("Embedding size %u \n", size);
    int start = SIGNATURE_SIZE_BITS; // Start embedding after the signature
    int end = start + 32; // The size field is 32 bits

    for (int i = start; i < end; ++i) {
        unsigned int bit = (size >> (i - start)) & 1; // Adjust bit index by subtracting start
        setLSB(&pixels[i].blue, bit);
    }

    printf("SIZE1 %d\n", size);
}


void setLSB(unsigned char* byte, int bitValue) {
    // Ensure the bitValue is either 0 or 1
    bitValue = (bitValue != 0) ? 1 : 0;

    // Set or clear the LSB
    *byte = (*byte & 0xFE) | bitValue;
}

int getBit(const int* data, int size, int position) {
    int byteIndex = position / 32;
    int bitIndex = position % 32;

    if (byteIndex >= size) {
        fprintf(stderr, "Error: Bit position out of bounds.\n");
        return -1; // Indicate an error
    }

    return (data[byteIndex] >> bitIndex) & 1;
}

void embed12BitPayload(Pixel* pixels, int numPixels, const int* compressedPayload, int compressedSize) {
    int totalBits = compressedSize * 12; // 12 bits per code

    int bitPosition = 0;
    int startPixel = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS;


    for (int i = startPixel; bitPosition < totalBits; ++i) {
        if (i >= numPixels) {
            fprintf(stderr, "Error: Reached the end of the pixel array. i: %d, numPixels: %d\n", i, numPixels);
            break;
        }

        int payloadIndex = bitPosition / 12;
        int bitIndexInPayload = bitPosition % 12;
        int bit = (compressedPayload[payloadIndex] >> bitIndexInPayload) & 1;
        printf("Compressed %d",compressedPayload[payloadIndex] );

        setLSB(&pixels[i].blue, bit);
        bitPosition++;

        if (bitPosition >= totalBits) {
            printf("Successfully embedded all bits. Last bit position: %d\n", bitPosition);
            break;
        }
    }

    // Check if all bits were embedded
    if (bitPosition != totalBits) {
        fprintf(stderr, "Error: Not all payload bits were embedded. Embedded: %d, Expected: %d\n", bitPosition, totalBits);
    }
}

int* extract12BitPayload(const Pixel* pixels, int numPixels, int* compressedPayloadSize) {
    int startIndex = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS; // This should be 80



    unsigned int payloadBitSize = extractSizeFromPixelData(pixels, numPixels);


    *compressedPayloadSize = (payloadBitSize + 11) / 12; // Calculate the number of 12-bit codes

    int* payload = (int*)malloc(*compressedPayloadSize * sizeof(int));
    if (!payload) {
        fprintf(stderr, "Memory allocation failed for payload extraction.\n");
        return NULL;
    }

    memset(payload, 0, *compressedPayloadSize * sizeof(int));

    int bitPosition = 0;
    for (int i = startIndex; i < payloadBitSize; ++i) {
        int payloadIndex = bitPosition / 12;
        int bitIndexInPayload = bitPosition % 12;

        int bit = pixels[i].blue & 1;
        payload[payloadIndex] |= (bit << bitIndexInPayload);

        bitPosition++;
        if (bitPosition >= payloadBitSize) {
            break;
        }
    }

    return payload;
}