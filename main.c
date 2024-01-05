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

        printf("We will embed.\n");

        type = determine_type(image_name);
        printf("Type determined: %d\n", type);
        printf("++++++++++++++++++++");
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

            printf("i am in type = 2");
            if (embed_to_bmp(image_name, image_name, compressed_payload, compressed_size) != 0) {
                fprintf(stderr, "Failed to embed payload into image or create output image.\n");
                free(compressed_payload);
                return 3;
            }
            free(compressed_payload);

        }
        if(type==3){
            printf("going to embed to png\n");
            if (embed_to_png(image_name, image_name, compressed_payload, compressed_size) != 0) {
                fprintf(stderr, "Failed to embed payload into image or create output image.\n");
                return 3;
            }
        }
        printf("Embedding is done.\n");
    }

    if(strcmp(direction, "-x") == 0){
        printf("We will extract.\n");
        type = determine_type(image_name);
        printf("\n\ndecompressing\n\n");

        printf("filetype is %d", type);
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
        printf("Extraction is done.\n");
    }

    printf("I am done and everything is ok.\n");
    return 0;
}