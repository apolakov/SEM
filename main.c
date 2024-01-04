#include <stdio.h>
#include <stdlib.h>
#include "lzw.h"
#include "files.h"
#include "bmp.h"
#include "pngs.h"
#include "checksuma.h"

#define TABLE_SIZE 16384
#define MAX_CHAR 256


int main() {
    const char* image_name = "../png24.png";
    const char* payload_name = "../redkvicka.jpg";
    const char* output_name = "../output.png";
    const char* decompressed_name = "../decompressed_payload.jpg";
    generate_crc32_table();


    int type = 1; /* bmp=2, png=3 */


    int payload_size;
    unsigned char* payload_data;
    int* compressed_payload;
    int compressed_size;
    int extraction_success;

    /* Step 1: Determine file type and check if it's 24-bit */
    type = determine_type(image_name);
    if (type == 1) {
        fprintf(stderr, "File is not a 24-bit BMP or PNG.\n");
        return 2;
    }

    payload_data = read_payload(payload_name, &payload_size);
    if (!payload_data) {
        fprintf(stderr, "Failed to load payload data.\n");
        return 1;
    }

    compressed_payload = lzw_compress(payload_data, payload_size, &compressed_size);
    free(payload_data); // Assume payload_data is no longer needed after this point
    if (!compressed_payload) {
        fprintf(stderr, "Compression failed.\n");
        return 1;
    }


    if(type==2){

        if (embed_to_bmp(image_name, output_name, compressed_payload, compressed_size, payload_name) != 0) {
            fprintf(stderr, "Failed to embed payload into image or create output image.\n");
            free(compressed_payload);
            return 3;
        }
        free(compressed_payload);

        extraction_success = extract_payload(output_name, decompressed_name);
        if (extraction_success != 0) {
            fprintf(stderr, "Failed to extract and decompress payload from image.\n");
            return extraction_success;
        }
    }
    if(type==3){
        printf("Original Playload size in byets: %d\n", payload_size);
        if (embed_to_png(image_name, output_name, compressed_payload, compressed_size) != 0) {
            fprintf(stderr, "Failed to embed payload into image or create output image.\n");
            return 3;
        }

        extraction_success = extract_for_png(output_name, decompressed_name);
        if (extraction_success != 0) {
            fprintf(stderr, "Failed to extract and decompress payload from image.\n");
            return extraction_success;
        }
    }

    return 0;
}

int comparePayloads(const int* payload1, const int* payload2, int size) {
    for (int i = 0; i < size; i++) {
        if (payload1[i] != payload2[i]) {
            printf("Mismatch at index %d: %d != %d\n", i, payload1[i], payload2[i]);
            return 0; // Return 0 if mismatch
        }
    }
    return 1; // Return 1 if match
}




/*



// Function to compare two blocks of memory




int writeBinaryPayloadData(const char* filename, const unsigned char* data, int size) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing payload.\n");
        return 1;
    }

    fwrite(data, 1, size, file);
    fclose(file);
    return 0;
}

void printData(const unsigned char* data, int size, const char* label) {
    printf("%s - First few bytes: ", label);
    for (int i = 0; i < size; ++i) {
        printf("%02x ", data[i]);
    }

    printf("\n");
}
 */