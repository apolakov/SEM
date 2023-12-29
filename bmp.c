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


    // Open the BMP file
    FILE *inputFile = fopen(imageFilename, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open BMP file.\n");
        return 1;
    }

    // Read the headers
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, inputFile);
    fread(&bih, sizeof(BITMAPINFOHEADER), 1, inputFile);

    printf("Image Width: %d, Height: %d, Bits Per Pixel: %d\n", bih.width, bih.height, bih.bits);

    // Calculate the row padding
    int padding = (4 - (bih.width * sizeof(Pixel)) % 4) % 4;

    // Calculate the total pixel data size, including padding
    int pixelDataSize = bih.width * abs(bih.height) * sizeof(Pixel) + padding * abs(bih.height);
    int actualPixelDataSize = bih.width * abs(bih.height) * sizeof(Pixel); // Excluding padding

    Pixel *pixels = readPixelData(inputFile, bfh, bih, &pixelDataSize);
    fclose(inputFile);
    if (!pixels) {
        fprintf(stderr, "Failed to read pixel data.\n");
        return 1;
    }

    int numPixels = bih.width * abs(bih.height);

    printf("Size of int: %zu bytes\n", sizeof(int));
    printf("CompressedSize I am using in line : int payloadBits = compressedSize * sizeof(int) * 8; is : %d\n", compressedSize);

    int payloadBits = compressedSize *12; // Payload size in bits
    printf("Compressed payload size is %d \n", payloadBits/1024);
    int availableBits = numPixels - 32; // Available bits for payload, minus 32 for the size
    printf("Payload size in bits (payloadBits): %d\n", payloadBits);

    if (payloadBits > availableBits) {
        fprintf(stderr, "Not enough space in the image for the payload. Payload is %d and avaliable is %d\n",payloadBits,availableBits);
        free(pixels);
        return 1;
    }

    // Embed payload size and payload into the image
    printf("Embedding payload size: %d bits\n", payloadBits);
    embedSize(pixels, payloadBits);

    int totalBits = payloadBits; // Define totalBits based on payloadBits

    embed12BitPayload(pixels, numPixels, compressedPayload, compressedSize);


    // Save the modified image
    if (!saveImage(outputImageFilename, bfh, bih, (unsigned char*)pixels, pixelDataSize)) {
        fprintf(stderr, "Failed to create output image with embedded payload.\n");
        free(pixels);
        return 1;
    }
    printf("Image Width: %d, Height: %d, Bits Per Pixel: %d\n", bih.width, bih.height, bih.bits);

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
    int* compressedPayload = extract12BitPayload(pixels, (pixelDataSize / sizeof(Pixel)), &compressedPayloadSize);
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

/*
int* extractPayload(const Pixel* pixels, int numPixels, int* compressedPayloadSize) {
    if (numPixels < 32) {
        fprintf(stderr, "Not enough pixels to extract the payload size.\n");
        return NULL;
    }

    // Extract the size of the payload in bits from the first 32 pixels
    unsigned int payloadBitSize = extractSizeFromPixelData(pixels, numPixels);
    printf("Extracted payload size: %u bits\n", payloadBitSize);

    // Calculate the size of the compressed payload in integers
    // The bit size needs to be rounded up to the nearest multiple of 32 bits (size of int)
    *compressedPayloadSize = (payloadBitSize + 31) / 32; // Rounded up to the nearest integer
    printf("Compressed payload size (in integers, after rounding up): %d\n", *compressedPayloadSize);

    // Allocate memory for the payload based on the extracted size
    int* payload = (int*)malloc(*compressedPayloadSize * sizeof(int));
    if (!payload) {
        fprintf(stderr, "Memory allocation failed for payload extraction.\n");
        return NULL;
    }

    // Clear the memory area for the payload
    memset(payload, 0, *compressedPayloadSize * sizeof(int));

    // Extract the payload bits from the pixels starting from pixel 33 (index 32)
    int bitPosition = 0; // Bit position within an integer
    for (int i = 32, payloadIndex = 0; i < numPixels && payloadIndex < *compressedPayloadSize; ++i) {
        int bit = pixels[i].blue & 1; // Extract the LSB of the blue channel
        payload[payloadIndex] |= (bit << bitPosition); // Set the bit at the correct position
        bitPosition++;

        // If we have filled one integer, move to the next one
        if (bitPosition == 32) {
            bitPosition = 0;
            payloadIndex++;
        }
    }

    return payload; // Return the pointer to the extracted payload
}
 */


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
        fprintf(stderr, "Error: Bit position out of bounds.\n");
        return -1; // Indicate an error
    }

    return (data[byteIndex] >> bitIndex) & 1;
}

void embed12BitPayload(Pixel* pixels, int numPixels, const int* compressedPayload, int compressedSize) {
    int totalBits = compressedSize * 12; // 12 bits per code

    int bitPosition = 0;
    for (int i = 32; bitPosition < totalBits; ++i) {
        if (i >= numPixels) {
            fprintf(stderr, "Error: Reached the end of the pixel array. i: %d, numPixels: %d\n", i, numPixels);
            break;
        }

        int payloadIndex = bitPosition / 12;
        int bitIndexInPayload = bitPosition % 12;
        int bit = (compressedPayload[payloadIndex] >> bitIndexInPayload) & 1;

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
    unsigned int payloadBitSize = extractSizeFromPixelData(pixels, numPixels);

    *compressedPayloadSize = (payloadBitSize + 11) / 12; // Calculate the number of 12-bit codes

    int* payload = (int*)malloc(*compressedPayloadSize * sizeof(int));
    if (!payload) {
        fprintf(stderr, "Memory allocation failed for payload extraction.\n");
        return NULL;
    }

    memset(payload, 0, *compressedPayloadSize * sizeof(int));

    int bitPosition = 0;
    for (int i = 32; i < numPixels; ++i) {
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
