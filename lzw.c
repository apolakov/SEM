#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "lzw.h"

#define MAX_CHAR 256
#define TABLE_SIZE 4096


dictionary_entry* table[TABLE_SIZE];
int next_available_code = 0;

int* lzw_compress(const unsigned char* input, int size, int* output_size) {
    unsigned char current_string[MAX_CHAR + 1] = {0};
    int current_length = 0;
    int* output;
    int outputIndex = 0;
    int last_found = -1;
    int i, code;

    initialize_dictionary();

    output = (int*)malloc(TABLE_SIZE * sizeof(int));
    if (!output) {
        fprintf(stderr, "Memory allocation failed for output array.\n");
        return NULL;
    }

    for (i = 0; i < size; i++) {
        current_string[current_length] = input[i];
        current_length++;

        code = find_bytes_code(current_string, current_length);
        if (code != -1) {
            last_found = code;
        } else {
            if (last_found != -1) {
                output[outputIndex++] = last_found;
            }
            if (current_length <= MAX_CHAR) {
                bytes_to_dictionary(current_string, current_length);
            }
            current_string[0] = input[i];
            current_length = 1;
            last_found = find_bytes_code(current_string, current_length);
        }
        if (outputIndex >= TABLE_SIZE) {
            int newSize = outputIndex * 2;
            int* temp = realloc(output, newSize * sizeof(int));
            if (!temp) {
                fprintf(stderr, "Memory reallocation failed during compression.\n");
                free(output);
                free_dictionary();
                return NULL;
            }
            output = temp;
        }
    }

    if (last_found != -1) {
        output[outputIndex++] = last_found;
    }

    *output_size = outputIndex;
    free_dictionary();
    return output;
}


unsigned char* lzw_decompress(const int* codes, int size, int* decompressedSize) {
    int bufferSize = 1024; /* Initial buffer size */
    unsigned char* decompressed;
    unsigned char* temp;
    unsigned char* newString;
    int outputIndex = 0;
    int i, code, newLength, nextCode;

    initialize_dictionary();

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

        if (code < 0 || code >= next_available_code) {
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
            if (nextCode < next_available_code) {
                newString[newLength - 1] = table[nextCode]->bytes[0];
            } else if (nextCode == next_available_code) {
                newString[newLength - 1] = newString[0];
            } else {
                fprintf(stderr, "Next code %d is out of bounds at index %d\n", nextCode, i);
                goto error;
            }

            bytes_to_dictionary(newString, newLength);
        }
    }

    *decompressedSize = outputIndex;
    free(newString);
    free_dictionary();
    printf("Decompressing -> end\n");
    return decompressed;

    error:
    free(decompressed);
    free(newString);
    free_dictionary();
    return NULL;
}

void bytes_to_dictionary(const unsigned char* bytes, int length) {

    if (next_available_code >= TABLE_SIZE) {
        fprintf(stderr, "Dictionary is full, resetting.\n");
        renew_dictionary();
        printf("Dictionary reset and reinitialized.\n");

    }

    table[next_available_code] = (dictionary_entry*)malloc(sizeof(dictionary_entry));
    if (table[next_available_code] == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return;
    }

    table[next_available_code]->bytes = (unsigned char*)malloc(length);
    if (table[next_available_code]->bytes == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(table[next_available_code]);
        return;
    }

    memcpy(table[next_available_code]->bytes, bytes, length);
    table[next_available_code]->length = length;
    table[next_available_code]->code = next_available_code;
    next_available_code++;
}

void initialize_dictionary() {
    int i;
    for (i = 0; i < MAX_CHAR; i++) {
        table[i] = (dictionary_entry*)malloc(sizeof(dictionary_entry));
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
    next_available_code = MAX_CHAR;
}


int find_bytes_code(const unsigned char* bytes, int length) {
    int i;
    for (i = 0; i < next_available_code; i++) {
        if (table[i] && table[i]->length == length && memcmp(table[i]->bytes, bytes, length) == 0) {
            return table[i]->code;
        }
    }
    return -1;
}

void free_dictionary() {
    int i;
    for (i = 0; i < TABLE_SIZE; i++) {
        if (table[i]) {
            free(table[i]->bytes);
            free(table[i]);
            table[i] = NULL;
        }
    }
    next_available_code = 0;
}

unsigned char* read_payload(const char* filename, int* size) {
    FILE* file;
    unsigned char* data;
    size_t current;

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

    current = fread(data, 1, *size, file);
    if (current != (size_t)*size) {
        fprintf(stderr, "Error reading from file.\n");
        free(data);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return data;
}

void reset_dictionary() {
    int i;
    for (i = 0; i < TABLE_SIZE; i++) {
        if (table[i]) {
            free(table[i]->bytes);
            free(table[i]);
            table[i] = NULL;
        }
    }
    next_available_code = MAX_CHAR;
}

void renew_dictionary() {
    reset_dictionary();
    initialize_dictionary();
}