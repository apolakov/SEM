#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
//#include "function.h"
#include <stdlib.h>
#include "lzw.h"
#include "bmp.h"

#define MAX_CHAR 256
#define TABLE_SIZE 4096


DictionaryEntry* table[TABLE_SIZE];
int nextAvailableCode = 0;

int* lzwCompress(const unsigned char* input, int size, int* outputSize) {
    initializeDictionary();

    printf("Compress 20");
    unsigned char currentString[MAX_CHAR + 1] = {0};
    int currentLength = 0;
    int* output = (int*)malloc(TABLE_SIZE * sizeof(int));
    int outputIndex = 0;
    printf("Compress 25");
    int lastFoundCode = -1;
    for (int i = 0; i < size; i++) {
        printf("%d\n",i);
        currentString[currentLength] = input[i];
        currentLength++;

        int code = findBytesCode(currentString, currentLength);
        if (code != -1) {
            lastFoundCode = code;
        } else {
            if (lastFoundCode != -1) {
                output[outputIndex++] = lastFoundCode;
            }
            if (currentLength <= MAX_CHAR) {
                addBytesToDictionary(currentString, currentLength);
            }
            currentString[0] = input[i];
            currentLength = 1;
            lastFoundCode = findBytesCode(currentString, currentLength);
        }
    }
    printf("Compress 46");

    if (lastFoundCode != -1) {
        output[outputIndex++] = lastFoundCode;
    }

    *outputSize = outputIndex;
    freeDictionary();
    return output;
}



unsigned char* lzwDecompress(const int* codes, int size, int* decompressedSize) {

    printf("decompressing -> start\n");
    printf("decompressing -> initializeDictionary()\n");
    initializeDictionary();

    int bufferSize = 1024; // Initial buffer size
    unsigned char* decompressed = (unsigned char*)malloc(bufferSize);
    int outputIndex = 0;

    for (int i = 0; i < size; i++) {
        int code = codes[i];
        if (code < 0 || code >= nextAvailableCode) {
            free(decompressed);
            freeDictionary();
            return NULL; // Invalid code
        }

        if (outputIndex + table[code]->length >= bufferSize) {
            bufferSize *= 2;
            decompressed = realloc(decompressed, bufferSize);
        }

        memcpy(decompressed + outputIndex, table[code]->bytes, table[code]->length);
        outputIndex += table[code]->length;

        // Add new entry to dictionary
        if (i + 1 < size) {
            unsigned char newString[MAX_CHAR + 1];
            int newLength = table[code]->length + 1;
            memcpy(newString, table[code]->bytes, table[code]->length);
            newString[newLength - 1] = table[codes[i+1]]->bytes[0];
            addBytesToDictionary(newString, newLength);
        }
    }
    printf("decompressing -> after for()\n");

    *decompressedSize = outputIndex;
    printf("decompressing -> after decompressedSize()\n");
    freeDictionary();
    printf("decompressing -> after freeing dictionary\n");
    printf("decompressing -> end\n");
    return decompressed;

}


void addBytesToDictionary(const unsigned char* bytes, int length) {
    if (nextAvailableCode >= TABLE_SIZE) {
        return; // Dictionary full, cannot add more
    }
    table[nextAvailableCode] = (DictionaryEntry*)malloc(sizeof(DictionaryEntry));
    table[nextAvailableCode]->bytes = (unsigned char*)malloc(length);
    memcpy(table[nextAvailableCode]->bytes, bytes, length);
    table[nextAvailableCode]->length = length;
    table[nextAvailableCode]->code = nextAvailableCode;
    nextAvailableCode++;
}

// Function to initialize the dictionary for LZW compression
void initializeDictionary() {
    for (int i = 0; i < MAX_CHAR; i++) {
        table[i] = (DictionaryEntry*)malloc(sizeof(DictionaryEntry));
        table[i]->bytes = (unsigned char*)malloc(sizeof(unsigned char));
        table[i]->bytes[0] = (unsigned char)i;
        table[i]->length = 1;
        table[i]->code = i;
    }
    nextAvailableCode = MAX_CHAR;
}

int findBytesCode(const unsigned char* bytes, int length) {
    for (int i = 0; i < nextAvailableCode; i++) {
        if (table[i] && table[i]->length == length && memcmp(table[i]->bytes, bytes, length) == 0) {
            return table[i]->code;
        }
    }
    return -1;
}

void freeDictionary() {
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (table[i]) {
            free(table[i]->bytes);
            free(table[i]);
            table[i] = NULL; // Set to NULL after freeing
        }
    }
    nextAvailableCode = 0; // Reset the next available code
}

unsigned char* readBinaryPayloadData(const char* filename, int* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open payload file.\n");
        return NULL;
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the payload
    unsigned char* data = (unsigned char*)malloc(*size);
    if (!data) {
        fprintf(stderr, "Memory allocation failed for payload.\n");
        fclose(file);
        return NULL;
    }

    // Read the payload into the buffer
    fread(data, 1, *size, file);
    fclose(file);
    return data;
}

// Function to load the original compressed payload from a file
int* loadOriginalCompressedPayload(const char* filename, int* size) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening original compressed payload file");
        return NULL;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    *size = ftell(file) / sizeof(int);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the original compressed data
    int* data = (int*)malloc(*size * sizeof(int));
    if (data == NULL) {
        fprintf(stderr, "Memory allocation failed for original compressed payload.\n");
        fclose(file);
        return NULL;
    }

    // Load the data
    fread(data, sizeof(int), *size, file);
    fclose(file);
    return data;
}