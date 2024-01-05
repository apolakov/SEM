#include <stdio.h>
#include <string.h>
#include "checksuma.h"
#include "pngs.h"

#define SIGNATURE_SIZE_BYTES 6
#define POLYNOMIAL 0xEDB88320
unsigned long crc32_table[256];


void set_lsb_sign(unsigned char* byte, unsigned char bitValue) {
    *byte = (*byte & 0xFE) | (bitValue & 1);
}



void embed_signature(Pixel* pixels) {
    unsigned char signature[SIGNATURE_SIZE_BYTES] = {0x32, 0x30, 0x30, 0x32, 0x31, 0x32};
    int i, byteIndex, bitIndex;
    unsigned char bit;

    for (i = 0; i < SIGNATURE_SIZE_BYTES * 8; ++i) {
        byteIndex = i / 8;
        bitIndex = i % 8;
        bit = (signature[byteIndex] >> bitIndex) & 1;
        set_lsb_sign(&pixels[i].blue, bit);
    }
}

int check_signature(const Pixel* pixels) {
    unsigned char expectedSignature[SIGNATURE_SIZE_BYTES] = {0x32, 0x30, 0x30, 0x32, 0x31, 0x32};
    unsigned char extractedSignature[SIGNATURE_SIZE_BYTES] = {0};
    int i, byteIndex, bitIndex;
    unsigned char bit;

    /* Extract the signature */
    for (i = 0; i < SIGNATURE_SIZE_BYTES * 8; ++i) {
        byteIndex = i / 8;
        bitIndex = i % 8;
        bit = pixels[i].blue & 1;
        extractedSignature[byteIndex] |= (bit << bitIndex);
    }

    /* Print the extracted signature for debugging */
    printf("Extracted Signature: ");
    for (i = 0; i < SIGNATURE_SIZE_BYTES; ++i) {
        printf("%02X ", extractedSignature[i]);
    }
    printf("\n");

    /* Compare the extracted signature with the expected one */
    if (memcmp(extractedSignature, expectedSignature, SIGNATURE_SIZE_BYTES) == 0) {
        return 1; /* Signature matches */
    } else {
        printf("Signature mismatch.\nExpected Signature: ");
        for (i = 0; i < SIGNATURE_SIZE_BYTES; ++i) {
            printf("%02X ", expectedSignature[i]);
        }
        printf("\n");
        return 0; /* Signature does not match */
    }
}



void generate_crc32_table() {
    unsigned long i, j, crc;

    for (i = 0; i < 256; i++) {
        crc = i;
        for (j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
}


void embed_crc(Pixel* pixels, int width, int height, unsigned long crc, unsigned long payloadBitSize) {


    unsigned long imageCapacity = (unsigned long)width * height;
    unsigned long crcPosition, i, pixelIndex;
    unsigned char bit;

    crcPosition = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS + payloadBitSize; /* Adjusted CRC position */
    printf("Embedding CRC: %lu at position: %lu\n", crc, crcPosition);

    if (crcPosition + 32 > imageCapacity * 8) {
        fprintf(stderr, "Not enough space to embed CRC in image.\n");
        return;
    }

    for (i = 0; i < 32; ++i) {
        pixelIndex = crcPosition + i;
        bit = (crc >> (31 - i)) & 1;
        embed_bit(&pixels[pixelIndex], bit);
    }
}


unsigned long extract_crc(const Pixel* pixels, int width, int height, int compressedSizeBits) {
    unsigned long imageCapacity, crcPosition, i, pixelIndex;
    unsigned char bit;
    unsigned long crc = 0;
    unsigned long mask = 0xFFFFFFFF; // Mask to keep only the lower 32 bits

    imageCapacity = (unsigned long)width * height;
    crcPosition = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS + compressedSizeBits;

    if (crcPosition + 32 > imageCapacity * 8) {
        fprintf(stderr, "Not enough space in image to contain a CRC.\n");
        return 0;
    }

    for (i = 0; i < 32; ++i) {
        pixelIndex = crcPosition + i;
        bit = extract_bit(&pixels[pixelIndex]);
        crc |= (bit << (31 - i)) & mask;
    }

    return crc;
}



unsigned long calculate_crc(const int* bitArray, int bitArraySize) {
    unsigned long crc = 0xFFFFFFFF;
    int i, bit;

    for (i = 0; i < bitArraySize; ++i) {
        bit = (bitArray[i / 32] >> (i % 32)) & 1;
        crc = crc32_table[(crc ^ bit) & 0x01] ^ (crc >> 1);
    }
    printf("Calculating %lu\n", crc ^ 0xFFFFFFFF);
    return crc ^ 0xFFFFFFFF;
}

