#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
//#include "function.h"
#include <stdlib.h>
#include "lzw.h"
#include "bmp.h"

#define MAX_CHAR 256
#define TABLE_SIZE 300000


DictionaryEntry* table[TABLE_SIZE];
int nextAvailableCode = 0;

int* lzwCompress(const unsigned char* input, int size, int* outputSize) {
    unsigned char currentString[MAX_CHAR + 1] = {0};
    int currentLength = 0;
    int* output;
    int outputIndex = 0;
    int lastFoundCode = -1;
    int i, code;

    initializeDictionary();

    output = (int*)malloc(TABLE_SIZE * sizeof(int));
    if (!output) {
        fprintf(stderr, "Memory allocation failed for output array.\n");
        return NULL;
    }

    for (i = 0; i < size; i++) {
        currentString[currentLength] = input[i];
        currentLength++;

        code = findBytesCode(currentString, currentLength);
        if (code != -1) {
            lastFoundCode = code;
        } else {
            if (lastFoundCode != -1) {
                output[outputIndex++] = lastFoundCode;
                printf("Compressing: Code %d at index %d\n", lastFoundCode, outputIndex - 1); /* Debug statement */
            }
            if (currentLength <= MAX_CHAR) {
                addBytesToDictionary(currentString, currentLength);
            }
            currentString[0] = input[i];
            currentLength = 1;
            lastFoundCode = findBytesCode(currentString, currentLength);
        }
    }

    if (lastFoundCode != -1) {
        output[outputIndex++] = lastFoundCode;
        printf("Compressing: Code %d at index %d\n", lastFoundCode, outputIndex - 1); /* Debug statement */
    }

    *outputSize = outputIndex;
    freeDictionary();
    return output;
}


unsigned char* lzwDecompress(const int* codes, int size, int* decompressedSize) {
    int bufferSize = 1024; /* Initial buffer size */
    unsigned char* decompressed;
    unsigned char* temp;
    unsigned char* newString;
    int outputIndex = 0;
    int i, code, newLength, nextCode;

    printf("Decompressing -> start\n");
    initializeDictionary();

    decompressed = (unsigned char*)malloc(bufferSize);
    if (!decompressed) {
        fprintf(stderr, "Memory allocation failed at start of decompression.\n");
        return NULL;
    }

    newString = (unsigned char*)malloc(MAX_CHAR + 1);
    if (!newString) {
        fprintf(stderr, "Memory allocation failed for newString.\n");
        free(decompressed);
        return NULL;
    }

    for (i = 0; i < size; i++) {
        code = codes[i];
        printf("Decompressing: Code %d at index %d\n", code, i);

        if (code < 0 || code >= nextAvailableCode) {
            fprintf(stderr, "Found invalid code during decompression: %d\n", code);
            goto error;
        }

        if (outputIndex + table[code]->length >= bufferSize) {
            bufferSize *= 2;
            temp = realloc(decompressed, bufferSize);
            if (!temp) {
                fprintf(stderr, "Memory reallocation failed during decompression.\n");
                goto error;
            }
            decompressed = temp;
        }

        memcpy(decompressed + outputIndex, table[code]->bytes, table[code]->length);
        outputIndex += table[code]->length;

        if (i + 1 < size) {
            newLength = table[code]->length + 1;
            memcpy(newString, table[code]->bytes, table[code]->length);

            nextCode = codes[i + 1];
            if (nextCode < nextAvailableCode) {
                newString[newLength - 1] = table[nextCode]->bytes[0];
            } else if (nextCode == nextAvailableCode) {
                newString[newLength - 1] = newString[0];
            } else {
                fprintf(stderr, "Next code %d is out of bounds at index %d\n", nextCode, i);
                goto error;
            }

            addBytesToDictionary(newString, newLength);
        }
    }

    *decompressedSize = outputIndex;
    free(newString);
    freeDictionary();
    printf("Decompressing -> end\n");
    return decompressed;

    error:
    free(decompressed);
    free(newString);
    freeDictionary();
    return NULL;
}

void addBytesToDictionary(const unsigned char* bytes, int length) {
    if (nextAvailableCode >= TABLE_SIZE) {
        fprintf(stderr, "Dictionary is full, cannot add more entries.\n");
        return; /* Dictionary full, cannot add more */
    }

    table[nextAvailableCode] = (DictionaryEntry*)malloc(sizeof(DictionaryEntry));
    if (table[nextAvailableCode] == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return;
    }

    table[nextAvailableCode]->bytes = (unsigned char*)malloc(length);
    if (table[nextAvailableCode]->bytes == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(table[nextAvailableCode]);
        return;
    }

    memcpy(table[nextAvailableCode]->bytes, bytes, length);
    table[nextAvailableCode]->length = length;
    table[nextAvailableCode]->code = nextAvailableCode;
    nextAvailableCode++;
}

void initializeDictionary() {
    int i;
    for (i = 0; i < MAX_CHAR; i++) {
        table[i] = (DictionaryEntry*)malloc(sizeof(DictionaryEntry));
        if (table[i] == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
            return;
        }

        table[i]->bytes = (unsigned char*)malloc(sizeof(unsigned char));
        if (table[i]->bytes == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
            free(table[i]);
            return;
        }

        table[i]->bytes[0] = (unsigned char)i;
        table[i]->length = 1;
        table[i]->code = i;
    }
    nextAvailableCode = MAX_CHAR;
}


int findBytesCode(const unsigned char* bytes, int length) {
    int i;
    for (i = 0; i < nextAvailableCode; i++) {
        if (table[i] && table[i]->length == length && memcmp(table[i]->bytes, bytes, length) == 0) {
            return table[i]->code;
        }
    }
    return -1;
}

void freeDictionary() {
    int i;
    for (i = 0; i < TABLE_SIZE; i++) {
        if (table[i]) {
            free(table[i]->bytes);
            free(table[i]);
            table[i] = NULL;
        }
    }
    nextAvailableCode = 0;
}

unsigned char* readBinaryPayloadData(const char* filename, int* size) {
    FILE* file;
    unsigned char* data;
    size_t itemsRead;

    file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open payload file.\n");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    data = (unsigned char*)malloc(*size);
    if (!data) {
        fprintf(stderr, "Memory allocation failed for payload.\n");
        fclose(file);
        return NULL;
    }

    itemsRead = fread(data, 1, *size, file);
    if (itemsRead != (size_t)*size) {
        fprintf(stderr, "Error reading from file.\n");
        free(data);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return data;
}