#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "checksuma.h"
// Assuming Pixel is defined as

#define SIGNATURE_SIZE_BYTES 6
#define POLYNOMIAL 0xEDB88320


// Function to set a bit (LSB) in the blue channel
void setLSBSignature(uint8_t* byte, uint8_t bitValue) {
    *byte = (*byte & 0xFE) | (bitValue & 1);
}

// Function to get a bit (LSB) from the blue channel
uint8_t getLSBSignature(uint8_t byte) {
    return byte & 1;
}

void embedSignature(Pixel* pixels) {
    // Signature "200212" in ASCII
    uint8_t signature[6] = {0x32, 0x30, 0x30, 0x32, 0x31, 0x32};

    // Embed each bit of the signature into the LSB of the pixel's blue channel
    for (int i = 0; i < 6 * 8; ++i) { // 6 bytes * 8 bits per byte
        int byteIndex = i / 8;
        int bitIndex = i % 8;
        uint8_t bit = (signature[byteIndex] >> bitIndex) & 1;
        setLSBSignature(&pixels[i].blue, bit);
    }
}

int extractAndCheckSignature(const Pixel* pixels) {
    // Signature "200212" in ASCII
    uint8_t expectedSignature[SIGNATURE_SIZE_BYTES] = {0x32, 0x30, 0x30, 0x32, 0x31, 0x32};
    uint8_t extractedSignature[SIGNATURE_SIZE_BYTES] = {0};

    // Extract each bit of the signature from the LSB of the pixel's blue channel
    for (int i = 0; i < SIGNATURE_SIZE_BYTES * 8; ++i) { // 6 bytes * 8 bits per byte
        int byteIndex = i / 8;
        int bitIndex = i % 8;
        unsigned int bit = pixels[i].blue & 1;
        extractedSignature[byteIndex] |= (bit << bitIndex);
    }

    // Print extracted signature for debugging
    printf("Extracted Signature: ");
    for (int i = 0; i < SIGNATURE_SIZE_BYTES; ++i) {
        printf("%02X ", extractedSignature[i]);
    }
    printf("\n");

    // Check if the extracted signature matches the expected signature
    if (memcmp(extractedSignature, expectedSignature, SIGNATURE_SIZE_BYTES) == 0) {
        return 1; // Signature matches
    } else {
        // Print the mismatch notice
        printf("Signature mismatch.\nExpected Signature: ");
        for (int i = 0; i < SIGNATURE_SIZE_BYTES; ++i) {
            printf("%02X ", expectedSignature[i]);
        }
        printf("\n");
        return 0; // Signature does not match
    }
}

// Test functions for embedding and extracting the signature
/*
void testSignatureEmbedExtract() {
    Pixel testPixels[24] = {0}; // More than enough pixels
    uint8_t signature[3] = {0x20, 0x02, 0x12}; // Your signature
    uint8_t extractedSignature[3] = {0};

    // Embed the signature into the image
    embedSignature(testPixels, signature, 3);

    // Extract the signature from the image
    extractSignature(testPixels, extractedSignature, 3);

    // Check if the extracted signature matches the original
    if (memcmp(signature, extractedSignature, 3) == 0) {
        printf("Signature extracted successfully and matches the original.\n");
    } else {
        printf("Failed to extract the correct signature.\n");
    }
}

uint32_t crc32_table[256];

void generate_crc32_table() {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
}



uint32_t calculateCRC32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        uint8_t byte = data[i];
        crc = crc32_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}


void testCRC32() {
    const char* testString = "Hello, World!";
    uint32_t crc = calculateCRC32((const uint8_t*)testString, strlen(testString));
    printf("CRC32 for '%s' is 0x%08X\n", testString, crc);
    printf("CRC32 for '%s' is 0x%08X\n", testString, crc);
}

/*
uint32_t extractCRC(const Pixel* pixels, int numPixels) {
    // Assuming this function extracts the 32-bit CRC from the specified position in the pixel array
    // Implementation depends on how the CRC was embedded
    uint32_t crc = 0;
    for (int i = 0; i < CRC_SIZE_BITS; ++i) {
        int bit = pixels[i].blue & 1;
        crc |= (bit << i);
    }
    return crc;
}

 */

uint32_t extractCRC(const Pixel* pixels, int crcPosition) {
    uint32_t crc = 0;
    for (int i = 0; i < 32; ++i) {
        uint32_t bit = (pixels[crcPosition + i].blue & 1);
        crc |= (bit << i);
    }
    return crc;
}