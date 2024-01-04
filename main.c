#include <stdio.h>
#include <stdlib.h>
#include "lzw.h"
#include "files.h"
#include "bmp.h"
#include "pngs.h"
#include "checksuma.h"

#define TABLE_SIZE 16384
#define MAX_CHAR 256


int main(int argc, char *argv[]) {

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <image.png/bmp> <-h|-x> <payloadFile>\n", argv[0]);
        return 1;
    }

    const char* image_name = NULL;
    const char* direction = NULL;
    const char* payload_name = NULL;
   // const char* output_name = NULL;
   // const char* decompressed_name = NULL;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            direction = argv[i];
        } else if (!image_name) {
            image_name = argv[i];
        } else if (!payload_name) {
            payload_name = argv[i];
        }
    }
    int type; /* bmp=2, png=3 */
    int payload_size;
    unsigned char* payload_data;
    int* compressed_payload;
    int compressed_size;
    int extraction_success;


    generate_crc32_table();



    if (strcmp(direction, "-h") == 0){

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

            if (embed_to_bmp(image_name, image_name, compressed_payload, compressed_size) != 0) {
                fprintf(stderr, "Failed to embed payload into image or create output image.\n");
                free(compressed_payload);
                return 3;
            }
            free(compressed_payload);

        }
        if(type==3){
            if (embed_to_png(image_name, image_name, compressed_payload, compressed_size) != 0) {
                fprintf(stderr, "Failed to embed payload into image or create output image.\n");
                return 3;
            }
        }
    }

    if(strcmp(direction, "-x") == 0){
        type = determine_type(image_name);

        if(type==2){

            extraction_success = extract_payload(image_name, payload_name);
            if (extraction_success != 0) {
                fprintf(stderr, "Failed to extract and decompress payload from image.\n");
                return extraction_success;
            }
        }
        if(type==3){

            extraction_success = extract_for_png(image_name, payload_name);
            if (extraction_success != 0) {
                fprintf(stderr, "Failed to extract and decompress payload from image.\n");
                return extraction_success;
            }
        }
    }

    return 0;
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