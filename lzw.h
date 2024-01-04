#ifndef SEMESTRALKAPLSPLSPLS_LZW_H
#define SEMESTRALKAPLSPLSPLS_LZW_H

#include <stdint.h>

typedef struct {
    unsigned char* bytes;
    int length;
    int code;
} dictionary_entry;

void bytes_to_dictionary(const unsigned char* bytes, int length);
void initialize_dictionary();
int find_bytes_code(const unsigned char *bytes, int length);
void free_dictionary();
unsigned char* read_payload(const char* filename, int* size);
void reset_dictionary();
unsigned char* lzw_decompress(const int* codes, int size, int* decompressedSize);
int* lzw_compress(const unsigned char* input, int size, int* output_size);
void renew_dictionary();
#endif //SEMESTRALKAPLSPLSPLS_LZW_H
