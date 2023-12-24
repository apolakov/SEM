#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define TABLE_SIZE 16384
#define MAX_CHAR 256

/*
typedef struct {
    unsigned char *bytes;  // Byte array
    int length;            // Length of the byte array
    int code;              // Corresponding code
} DictionaryEntry;
*/







unsigned char* lzwDecompress(const int *codes, int size, int* decompressedSize);
int comparePayloads(const int* payload1, const int* payload2, int size);
int* loadOriginalCompressedPayload(const char* filename, int* size);

//int addBytesToDictionary(const unsigned char *bytes, int length);
//void embedPayload(unsigned char* pixelData, int pixelDataSize, const int* compressedPayload, int compressedSize);
//void extractPayload(unsigned char* pixelData, int pixelDataSize, int* extractedPayload, int compressedSize);
//unsigned char* readPixelData(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, int* pixelDataSize);
//int openAndValidateBMP(const char* filename, BITMAPFILEHEADER* bfh, BITMAPINFOHEADER* bih);
// Function prototype for reading pixel data from a BMP file
//unsigned char* readPixelData(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, int* pixelDataSize);
//int* extractPayload(unsigned char* pixelData, int pixelDataSize, int* compressedPayloadSize);
//unsigned int extractSizeFromPixelData(unsigned char* pixelData, int numPixels);
//void saveDecompressedPayload(const char* baseFilename, const unsigned char* decompressedPayload, int decompressedPayloadSize);
//void saveDecompressedPayload(const unsigned char* decompressedPayload, int decompressedPayloadSize);
//int compareFiles(const char *file1, const char *file2);
//int embedPayloadInImage(const char* imageFilename, const char* outputImageFilename, const int* compressedPayload, int compressedSize);
//int extractAndDecompressPayload(const char* inputImageFilename, const char* outputPayloadFilename);
//void addBytesToDictionary(const unsigned char *bytes, int length);
//int* lzwCompress(const unsigned char *input, int *size);
//unsigned char* getBytesFromCode(int code, int *length);
int analyze_file_format(const char *filename);
void check_file_type(const char *filename);
void* readFile(const char* filename, size_t* length);

int findStringCode(char *str);
void addStringToDictionary(char *str, int code);
int hashFunction(char *str);
char* getStringFromCode(int code);
unsigned char* readPayloadData(const char* filename, int* size);
//int* extractPayload(Pixel* pixels, int numPixels, int* compressedPayloadSize);


#endif