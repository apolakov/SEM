#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "lzw.h"


#define MAX_CHAR 256
#define TABLE_SIZE 30000


DictionaryEntry* table[TABLE_SIZE];
int nextAvailableCode = 0;



int* lzwCompress(const unsigned char* input, int size, int* outputSize) {
    // Initialize the dictionary
    initializeDictionary();

    // Prepare for compression
    int* output = (int*)malloc(size * sizeof(int)); // Allocate memory for worst-case scenario
    if (!output) {
        fprintf(stderr, "Memory allocation failed for output.\n");
        return NULL;
    }
    int outputIndex = 0;

    // Start compression
    int i = 0;
    while (i < size) {
        // Find the longest sequence in the dictionary
        int sequenceLength = 1;
        int code = -1;
        while (i + sequenceLength <= size) {
            int newCode = findBytesCode(&input[i], sequenceLength);
            if (newCode != -1) {
                code = newCode;
                sequenceLength++;
            } else {
                break;
            }
        }
        sequenceLength--;

        // Output the code for the longest found sequence
        if (code != -1) {
            output[outputIndex++] = code;
        }

        // Add new sequence to the dictionary
        if (i + sequenceLength < size) {
            addBytesToDictionary(&input[i], sequenceLength + 1);
        }

        i += sequenceLength;
    }

    // Set the output size
    *outputSize = outputIndex;

    // Resize output array to actual size
    output = (int*)realloc(output, (*outputSize) * sizeof(int));
    if (!output) {
        fprintf(stderr, "Memory allocation failed for output resizing.\n");
        return NULL;
    }

    // Clean up dictionary
    freeDictionary();

    return output;
}


unsigned char* lzwDecompress(const int* codes, int size, int* decompressedSize) {
    printf("Decompressing -> start\n");
    initializeDictionary();

    int bufferSize = 1024; // Initial buffer size
    unsigned char* decompressed = (unsigned char*)malloc(bufferSize);
    if (!decompressed) {
        fprintf(stderr, "Memory allocation failed at start of decompression.\n");
        return NULL;
    }
    int outputIndex = 0;

    for (int i = 0; i < size; i++) {
        int code = codes[i];
        //printf("Decompressing: Code %d at index %d\n", code, i);

        if (code < 0 || code >= nextAvailableCode) {
            free(decompressed);
            freeDictionary();
            fprintf(stderr, "Found invalid code during decompression: %d\n", code);
            return NULL;
        }

        if (outputIndex + table[code]->length >= bufferSize) {
            bufferSize *= 4;  // Increase the buffer size more aggressively
            unsigned char* temp = realloc(decompressed, bufferSize);
            if (!temp) {
                free(decompressed);
                freeDictionary();
                fprintf(stderr, "Memory reallocation failed during decompression.\n");
                return NULL;
            }
            decompressed = temp;
        }

        memcpy(decompressed + outputIndex, table[code]->bytes, table[code]->length);
        outputIndex += table[code]->length;

        if (i + 1 < size) {
            unsigned char newString[MAX_CHAR + 1];
            int newLength = table[code]->length + 1;
            memcpy(newString, table[code]->bytes, table[code]->length);

            int nextCode = codes[i + 1];
            if (nextCode < nextAvailableCode) {
                newString[newLength - 1] = table[nextCode]->bytes[0]; // Regular case
            } else if (nextCode == nextAvailableCode) {
                newString[newLength - 1] = newString[0]; // Special case
            } else {
                fprintf(stderr, "Next code %d is out of bounds at index %d\n", nextCode, i);
                free(decompressed);
                freeDictionary();
                return NULL;
            }

            addBytesToDictionary(newString, newLength);
        }
    }

    *decompressedSize = outputIndex;
    freeDictionary();
    printf("Decompressing -> end\n");
    return decompressed;
}

void clearDictionary() {
    for (int i = MAX_CHAR; i < nextAvailableCode; i++) {
        if (table[i]) {
            free(table[i]->bytes);
            free(table[i]);
            table[i] = NULL;
        }
    }
    nextAvailableCode = MAX_CHAR;
}

void addBytesToDictionary(const unsigned char* bytes, int length) {
    if (nextAvailableCode >= TABLE_SIZE) {
        clearDictionary(); // Clear and reinitialize the dictionary
        fprintf(stderr, "Dictionary was full and has been cleared.\n");
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
    size_t itemsRead = fread(data, 1, *size, file);
    if (itemsRead != *size) {
        fprintf(stderr, "Error reading from file.\n");
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    return data;
}

/*
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
    size_t itemsRead = fread(data, sizeof(int), *size, file);
    if (itemsRead != *size) {
        fprintf(stderr, "Error reading from file.\n");
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    return data;
}
 */