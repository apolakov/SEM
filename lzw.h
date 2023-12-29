#ifndef SEMESTRALKAPLSPLSPLS_LZW_H
#define SEMESTRALKAPLSPLSPLS_LZW_H

#include <stdint.h> // Include this for uint8_t and uint16_t

typedef struct {
    unsigned char* bytes;
    int length;
    int code; // If you're using 12-bit codes, this should be uint16_t
} DictionaryEntry;

void addBytesToDictionary(const unsigned char* bytes, int length);
void initializeDictionary();
int findBytesCode(const unsigned char *bytes, int length);
void freeDictionary();
unsigned char* readBinaryPayloadData(const char* filename, int* size);
int* loadOriginalCompressedPayload(const char* filename, int* size);
void resetDictionary();
void addBytesToDictionaryWithPrevCode(uint16_t prevCode, unsigned char nextByte);
void clearDictionary();

// Function prototypes
//uint8_t* lzwCompress(const unsigned char* input, int size, int* outputSize);
//unsigned char* lzwDecompress(const uint8_t* codes, int size, int* decompressedSize);
unsigned char* lzwDecompress(const int* codes, int size, int* decompressedSize);
int* lzwCompress(const unsigned char* input, int size, int* outputSize);
void resetAndReinitializeDictionary();
#endif //SEMESTRALKAPLSPLSPLS_LZW_H
