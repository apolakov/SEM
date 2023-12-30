#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Assuming Pixel is defined as
typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} Pixel;

// Function to set a bit (LSB) in the blue channel
void setLSBSignature(uint8_t* byte, uint8_t bitValue) {
    *byte = (*byte & 0xFE) | (bitValue & 1);
}

// Function to get a bit (LSB) from the blue channel
uint8_t getLSBSignature(uint8_t byte) {
    return byte & 1;
}

void embedSignature(Pixel* pixels, const uint8_t* signature, int signatureSize) {
    for (int i = 0; i < signatureSize * 8; ++i) {
        int byteIndex = i / 8;
        int bitIndex = i % 8;
        unsigned int bit = (signature[byteIndex] >> bitIndex) & 1;
        setLSBSignature(&pixels[i].blue, bit);
    }
}

void extractSignature(const Pixel* pixels, uint8_t* extractedSignature, int signatureSize) {
    for (int i = 0; i < signatureSize * 8; ++i) {
        int byteIndex = i / 8;
        int bitIndex = i % 8;
        unsigned int bit = pixels[i].blue & 1;
        extractedSignature[byteIndex] |= (bit << bitIndex);
    }
}


// Test functions for embedding and extracting the signature
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
