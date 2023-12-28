
#ifndef SEMESTRALKAPLSPLSPLS_LZW_H
#define SEMESTRALKAPLSPLSPLS_LZW_H

typedef struct {
    unsigned char* bytes;
    int length;
    int code;
} DictionaryEntry;


int* lzwCompress(const unsigned char *input, int size, int* outputSize);
unsigned char* lzwDecompress(const int *codes, int size, int* decompressedSize);
void addBytesToDictionary(const unsigned char* bytes, int length);
void initializeDictionary();
int findBytesCode(const unsigned char *bytes, int length);
void freeDictionary();
unsigned char* readBinaryPayloadData(const char* filename, int* size);
int* loadOriginalCompressedPayload(const char* filename, int* size);
void resetDictionary();












#endif //SEMESTRALKAPLSPLSPLS_LZW_H
