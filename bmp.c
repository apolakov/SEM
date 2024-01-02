#include "files.h"
#include <stdio.h>
#include <string.h>
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
    FILE* inputFile;
    int padding, pixelDataSize, numPixels, payloadBits, availableBits, crcBits;
    uint32_t crc;
    Pixel* pixels;

    inputFile = fopen(imageFilename, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open BMP file.\n");
        return 1;
    }

    readBitmapHeaders(inputFile, &bfh, &bih);

    padding = calculatePadding(bih);
    pixelDataSize = bih.width * abs(bih.height) * sizeof(Pixel) + padding * abs(bih.height);

    pixels = readPixelData(inputFile, bfh, bih, &pixelDataSize);
    if (!pixels) {
        fprintf(stderr, "Failed to read BMP file: %s\n", imageFilename);
        fclose(inputFile);
        return 1;
    }

    numPixels = bih.width * abs(bih.height);
    payloadBits = compressedSize * 12; /* Payload size in bits */
    availableBits = numPixels - SIZE_FIELD_BITS - SIGNATURE_SIZE_BITS; /* Available bits for payload, minus 32 for the size */

    if (payloadBits > availableBits) {
        fprintf(stderr, "Not enough space in the image for the payload. Payload is %d bits and available is %d bits\n", payloadBits, availableBits);
        free(pixels);
        return 1;
    }

    embedSignature(pixels);
    embedSize(pixels, payloadBits);
    embed12BitPayload(pixels, numPixels, compressedPayload, compressedSize);

    crc = calculateCRC32FromBits(compressedPayload, payloadBits);

    crcBits = 32;
    if (payloadBits + crcBits > availableBits) {
        fprintf(stderr, "Not enough space in the image for the CRC.\n");
        free(pixels);
        return 1;
    }

    embedCRCInPixels(pixels, bih.width, abs(bih.height), crc, payloadBits);

    if (!saveImage(outputImageFilename, bfh, bih, (unsigned char*)pixels, pixelDataSize)) {
        fprintf(stderr, "Failed to create output image with embedded payload.\n");
        free(pixels);
        return 1;
    }

    free(pixels);
    return 0;
}


void readBitmapHeadersAndPixelData(const char* filename, BITMAPFILEHEADER* bfh, BITMAPINFOHEADER* bih, Pixel** pixels, int* pixelDataSize) {
    FILE* inputFile;

    inputFile = fopen(filename, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open BMP file.\n");
        exit(1);
    }

    readBitmapHeaders(inputFile, bfh, bih);
    *pixels = readPixelData(inputFile, *bfh, *bih, pixelDataSize);
    fclose(inputFile);
}

void saveDecompressedPayload(const unsigned char* decompressedPayload, int size, const char* filename) {
    FILE* outputFile;

    outputFile = fopen(filename, "wb");
    if (!outputFile) {
        fprintf(stderr, "Failed to open output file for decompressed payload.\n");
        free((unsigned char*)decompressedPayload); /* Casting to unsigned char* is more appropriate */
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
    int compressedPayloadSize;
    int* compressedPayload;
    unsigned long extractedCRC, calculatedCRC; /* Replacing uint32_t for ANSI C compliance */
    int decompressedPayloadSize;
    unsigned char* decompressedPayload;

    /* Open input file and read bitmap headers and pixel data */
    readBitmapHeadersAndPixelData(inputImageFilename, &bfh, &bih, &pixels, &pixelDataSize);
    if (!pixels) {
        fprintf(stderr, "Failed to read pixel data.\n");
        return 1;
    }

    /* Check if the signature matches */
    if (!extractAndCheckSignature(pixels)) {
        fprintf(stderr, "Signature mismatch or not found.\n");
        free(pixels);
        return 1;
    }

    /* Extract compressed payload from image */
    compressedPayload = extract12BitPayload(pixels, (pixelDataSize / sizeof(Pixel)), &compressedPayloadSize);
    if (!compressedPayload) {
        fprintf(stderr, "Failed to extract payload.\n");
        free(pixels);
        return 1;
    }

    /* Extract and calculate CRC */
    extractedCRC = extractCRCFromPixels(pixels, bih.width, abs(bih.height), compressedPayloadSize * 12);
    calculatedCRC = calculateCRC32FromBits(compressedPayload, compressedPayloadSize * 12);

    /* Compare the extracted CRC with the calculated CRC */
    if (extractedCRC != calculatedCRC) {
        fprintf(stderr, "CRC mismatch. Payload may be corrupted.\n");
        free(compressedPayload);
        free(pixels);
        return 1;
    }
    free(pixels);

    /* Decompress the extracted payload */
    decompressedPayload = lzwDecompress(compressedPayload, compressedPayloadSize, &decompressedPayloadSize);
    free(compressedPayload);
    if (!decompressedPayload) {
        fprintf(stderr, "Failed to decompress payload.\n");
        return 1;
    }

    /* Save decompressed payload to file */
    saveDecompressedPayload(decompressedPayload, decompressedPayloadSize, outputPayloadBaseFilename);
    free(decompressedPayload);

    return 0;
}



unsigned int extractSizeFromPixelData(const Pixel* pixels, int numPixels) {
    int startIndex, i;
    unsigned int size, bit;

    startIndex = SIGNATURE_SIZE_BITS; /* Start index after the signature */

    if (numPixels < startIndex + 32) {
        fprintf(stderr, "Not enough pixels to extract payload size.\n");
        return 0;
    }

    size = 0;
    for (i = 0; i < 32; ++i) {
        bit = pixels[startIndex + i].blue & 1; /* Adjust index by adding startIndex */
        size |= (bit << i);
    }

    printf("Final extracted size: %u\n", size);
    return size;
}



Pixel* readPixelData(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, int* pixelDataSize) {
    int rowSize, padding, i;
    long totalSize;  /* Using long instead of long long for ANSI C compatibility */
    Pixel* pixelData;

    /* Calculate padding safely */
    rowSize = bih.width * sizeof(Pixel);
    padding = (4 - (rowSize % 4)) % 4;
    totalSize = (long)rowSize + padding; /* Cast to long for safety */
    totalSize *= abs(bih.height);

    /* Check for overflow or too large size */
    if (totalSize > INT_MAX) {
        fprintf(stderr, "Pixel data size too large.\n");
        fclose(file);
        return NULL;
    }

    *pixelDataSize = (int)totalSize;
    pixelData = (Pixel*)malloc(*pixelDataSize);
    if (!pixelData) {
        fprintf(stderr, "Memory allocation failed for pixel data.\n");
        fclose(file);
        return NULL;
    }

    /* Set the file position to the beginning of pixel data */
    fseek(file, bfh.offset, SEEK_SET);

    /* Read pixel data row by row */
    for (i = 0; i < abs(bih.height); ++i) {
        /* Read one row of pixel data */
        if (fread(pixelData + (bih.width * i), sizeof(Pixel), bih.width, file) != bih.width) {
            fprintf(stderr, "Failed to read pixel data.\n");
            free(pixelData);  /* Free allocated memory in case of read error */
            fclose(file);
            return NULL;
        }
        /* Skip over padding */
        fseek(file, padding, SEEK_CUR);
    }

    *pixelDataSize = bih.width * abs(bih.height) * sizeof(Pixel);
    return pixelData;
}

void setLSB(unsigned char* byte, int bitValue) {
    /* Ensure the bitValue is either 0 or 1 */
    bitValue = (bitValue != 0) ? 1 : 0;
    *byte = (*byte & 0xFE) | bitValue;
}

void embedSize(Pixel* pixels, unsigned int size) {
    int start, end, i;
    unsigned int bit;

    printf("Embedding size %u\n", size);

    start = SIGNATURE_SIZE_BITS; /* Start embedding after the signature */
    end = start + 32; /* The size field is 32 bits */

    for (i = start; i < end; ++i) {
        bit = (size >> (i - start)) & 1; /* Adjust bit index by subtracting start */
        setLSB(&pixels[i].blue, bit);
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

void embed12BitPayload(Pixel* pixels, int numPixels, const int* compressedPayload, int compressedSize) {
    int totalBits, bitPosition, startPixel, i, payloadIndex, bitIndexInPayload, bit;

    totalBits = compressedSize * 12; /* 12 bits per code */

    bitPosition = 0;
    startPixel = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS;

    for (i = startPixel; bitPosition < totalBits; ++i) {
        if (i >= numPixels) {
            fprintf(stderr, "Error: Reached the end of the pixel array. i: %d, numPixels: %d\n", i, numPixels);
            break;
        }

        payloadIndex = bitPosition / 12;
        bitIndexInPayload = bitPosition % 12;
        bit = (compressedPayload[payloadIndex] >> bitIndexInPayload) & 1;

        setLSB(&pixels[i].blue, bit);
        bitPosition++;

        if (bitPosition >= totalBits) {
            printf("Successfully embedded all bits. Last bit position: %d\n", bitPosition);
            break;
        }
    }

    /* Check if all bits were embedded */
    if (bitPosition != totalBits) {
        fprintf(stderr, "Error: Not all payload bits were embedded. Embedded: %d, Expected: %d\n", bitPosition, totalBits);
    }
}

int* extract12BitPayload(const Pixel* pixels, int numPixels, int* compressedPayloadSize) {
    int startIndex, payloadIndex, bitIndexInPayload, bit, bitPosition, i;
    unsigned int payloadBitSize;
    int* payload;

    startIndex = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS; /* This should be 80 */

    payloadBitSize = extractSizeFromPixelData(pixels, numPixels);

    *compressedPayloadSize = (payloadBitSize + 11) / 12; /* Calculate the number of 12-bit codes */

    payload = (int*)malloc(*compressedPayloadSize * sizeof(int));
    if (!payload) {
        fprintf(stderr, "Memory allocation failed for payload extraction.\n");
        return NULL;
    }

    memset(payload, 0, *compressedPayloadSize * sizeof(int));

    bitPosition = 0;
    for (i = startIndex; i < payloadBitSize; ++i) {
        payloadIndex = bitPosition / 12;
        bitIndexInPayload = bitPosition % 12;

        bit = pixels[i].blue & 1;
        payload[payloadIndex] |= (bit << bitIndexInPayload);

        bitPosition++;
        if (bitPosition >= payloadBitSize) {
            break;
        }
    }

    return payload;
}
