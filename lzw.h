#ifndef SEMESTRALKAPLSPLSPLS_LZW_H
#define SEMESTRALKAPLSPLSPLS_LZW_H

#include <stdint.h>

typedef struct {
    unsigned char* bytes;
    int length;
    int code;
} DictionaryEntry;

void addBytesToDictionary(const unsigned char* bytes, int length);
void initializeDictionary();
int findBytesCode(const unsigned char *bytes, int length);
void freeDictionary();
unsigned char* readBinaryPayloadData(const char* filename, int* size);
void resetDictionary();
unsigned char* lzwDecompress(const int* codes, int size, int* decompressedSize);
int* lzwCompress(const unsigned char* input, int size, int* outputSize);
void resetAndReinitializeDictionary();
#endif //SEMESTRALKAPLSPLSPLS_LZW_H
