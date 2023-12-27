#include "files.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
//#include "function.h"
#include <stdlib.h>
#include "lzw.h"
#include "bmp.h"


int embedPayloadInImage(const char* imageFilename, const char* outputImageFilename, const int* compressedPayload, int compressedSize, const char* payloadFilename) {
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    int pixelDataSize;

    // Open the BMP file
    FILE *inputFile = fopen(imageFilename, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open BMP file.\n");
        return 1;
    }

    // Read the headers
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, inputFile);
    fread(&bih, sizeof(BITMAPINFOHEADER), 1, inputFile);

    // Calculate the row padding
    int padding = (4 - (bih.width * 3) % 4) % 4;

    // Calculate the total pixel data size, including padding
    pixelDataSize = bih.width * abs(bih.height) * 3 + padding * abs(bih.height);

    Pixel *pixels = readPixelData(inputFile, bfh, bih, &pixelDataSize);
    fclose(inputFile);
    if (!pixels) {
        fprintf(stderr, "Failed to read pixel data.\n");
        return 1;
    }


    int numPixels =  bih.width * abs(bih.height);
    int payloadBits = compressedSize * sizeof(int) * 8; // Payload size in bits
    int availableBits = numPixels * 8 - 32; // Available bits for payload, minus 32 for the size


    // Debugging print statements to check sizes
    printf("Size of BITMAPFILEHEADER: %zu\n", sizeof(BITMAPFILEHEADER));
    printf("Size of BITMAPINFOHEADER: %zu\n", sizeof(BITMAPINFOHEADER));
    printf("Size of Pixel: %zu\n", sizeof(Pixel));
    printf("Number of pixels (numPixels): %d\n", numPixels);
    printf("Total bits available for payload (availableBits): %d\n", availableBits);
    printf("Total bits of payload (totalBits): %d\n", payloadBits);

    // Check if payload fits into the image
    if (payloadBits > availableBits) {
        fprintf(stderr, "Not enough space in the image for the payload.\n");
        free(pixels);
        return 1;
    }

    // Embed payload size and payload into the image
    int sizeToEmbed = payloadBits;
    embedSize(pixels, sizeToEmbed);
    embedPayload(pixels , numPixels , compressedPayload, compressedSize);

    // Save the modified image0
    if (!saveImage(outputImageFilename, bfh, bih, (unsigned char*)pixels, pixelDataSize)) {
        fprintf(stderr, "Failed to create output image with embedded payload.\n");
        free(pixels);
        return 1;
    }

    free(pixels);

    return 0;
}

int extractAndDecompressPayload(const char* inputImageFilename, const char* outputPayloadBaseFilename) {    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    int pixelDataSize;
    FILE* inputFile = fopen(inputImageFilename, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open BMP file.\n");
        return 1;
    }
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, inputFile);
    fread(&bih, sizeof(BITMAPINFOHEADER), 1, inputFile);
    printf("extractAndDecompressPayload-> start reading pixel data\n");
    Pixel* pixels = readPixelData(inputFile, bfh, bih, &pixelDataSize);
    fclose(inputFile);
    if (!pixels) {
        fprintf(stderr, "Failed to read pixel data.\n");
        return 1;
    }


    // Step 2: Extract compressed payload from pixel data
    int compressedPayloadSize;
    printf("extractAndDecompressPayload-> start extracting payload\n");
    int* compressedPayload = extractPayload(pixels , (pixelDataSize / sizeof(Pixel)), &compressedPayloadSize);
    free(pixels);
    if (!compressedPayload) {
        fprintf(stderr, "Failed to extract payload.\n");
        return 1;
    }

    // Step 3: Decompress payload
    printf("extractAndDecompressPayload-> start decompressing payload\n");
    int decompressedPayloadSize; // Variable to store the size of the decompressed data
    unsigned char* decompressedPayload = lzwDecompress(compressedPayload, compressedPayloadSize, &decompressedPayloadSize); // Modified call
    free(compressedPayload);
    if (!decompressedPayload) {
        fprintf(stderr, "Failed to decompress payload.\n");
        return 1;
    }

    // Step 4: Save the decompressed payload to a file
    printf("extractAndDecompressPayload-> start saving decompressed payload\n");
    FILE* outputFile = fopen(outputPayloadBaseFilename, "wb");
    if (!outputFile) {
        fprintf(stderr, "Failed to open output file for decompressed payload.\n");
        free(decompressedPayload);
        return 1;
    }
    fwrite(decompressedPayload, 1, decompressedPayloadSize, outputFile);
    fclose(outputFile);
    free(decompressedPayload);

    return 0; // Success
}


int* extractPayload(const Pixel* pixels, int numPixels, int* compressedPayloadSize) {

    unsigned int payloadBitSize = extractSizeFromPixelData(pixels, numPixels);
    printf("PAYLOADBITSIZE (before adding 31): %u\n", payloadBitSize);
    unsigned int adjustedSize = payloadBitSize + 31;
    printf("Adjusted size (PAYLOADBITSIZE + 31): %u\n", adjustedSize);
    *compressedPayloadSize = (payloadBitSize + 31)  / 32;
    printf("COMPRESSEDPAYLOADSIZE (after division): %d\n", *compressedPayloadSize);

    int* payload = (int*)malloc(*compressedPayloadSize * sizeof(int));
    if (!payload) {
        fprintf(stderr, "Memory allocation failed for payload extraction.\n");
        return NULL;
    }

    memset(payload, 0, *compressedPayloadSize * sizeof(int));


    // Start extracting from pixel 32, skipping the first 32 pixels used for the size
    for (int i = 32, payloadIndex = 0, bitPosition = 0; i < numPixels && payloadIndex < *compressedPayloadSize; ++i) {
        int bit = pixels[i].blue & 1;
        // Debugging statement
        printf("Extracted bit %d from pixel %d: %02X\n", bit, i, pixels[i].blue);
        payload[payloadIndex] |= (bit << bitPosition);
        bitPosition++;
        if (bitPosition == 32) {
            bitPosition = 0;
            payloadIndex++;
        }
    }



    return payload;
}

unsigned int extractSizeFromPixelData(const Pixel* pixels, int numPixels) {
    if (numPixels < 32) {
        fprintf(stderr, "Not enough pixels to extract payload size.\n");
        return 0;
    }

    unsigned int size = 0;
    for (int i = 0; i < 32; ++i) {
        unsigned int bit = pixels[i].blue & 1;
        size |= (bit << i);
        // You may want to print the intermediate values for debugging
        // printf("Extracting bit %d from pixel %d: %d, Cumulative size: %u\n", bit, i, pixels[i].blue, size);
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

void embedPayload(Pixel* pixels, int numPixels, const int* compressedPayload, int compressedSize) {
    compressedSize+=500;
    int totalBits = compressedSize * 32;
    int availableBits = (numPixels - 32) * 8; // Assuming first 32 pixels store metadata

    if (totalBits > availableBits) {
        fprintf(stderr, "Error: Not enough space in the image to embed the payload.\n");
        return;
    }

    // Start embedding the payload after the metadata pixels
    for (int i = 32, bitPosition = 0; i < numPixels && bitPosition < totalBits; ++i) {
        int bit = getBit(compressedPayload, compressedSize, bitPosition++);
        setLSB(&pixels[i].blue, bit); // Start after metadata pixels
        // Debugging statement
        printf("Embedded bit %d into pixel %d: %02X\n", bit, i, pixels[i].blue);
    }
}

// Embedding the size
void embedSize(Pixel* pixels, unsigned int size) {
    printf("embeded size %u \n",size);
    for (int i = 0; i < 32; ++i) {
        unsigned int bit = (size >> i) & 1;
        setLSB(&pixels[i].blue, bit);
    }
    size = size;
    printf("SIZE1 %d\n",size);
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
        return 0; // Out of bounds check
    }

    return (data[byteIndex] >> bitIndex) & 1;
}