#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "function.h"
#include "lzw.h"
#include "files.h"
#include "bmp.h"
#include "pngs.h"
#include <png.h>

#define TABLE_SIZE 16384
#define MAX_CHAR 256

//int writeBinaryPayloadData(const char* filename, const unsigned char* data, int size);
int compareImages(const char* originalFilename, const char* savedFilename);

int writeCompressedPayloadToFile(const char* filename, const int* compressedPayload, int compressedSize) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing compressed payload.\n");
        return 1;
    }

    // Calculate the total number of bytes needed for the 12-bit packed data
    int totalBytes = (compressedSize * 12 + 7) / 8;  // Round up to the nearest byte
    unsigned char* packedData = malloc(totalBytes);
    if (!packedData) {
        fprintf(stderr, "Memory allocation failed for packed data.\n");
        fclose(file);
        return 1;
    }

    // Pack the 12-bit codes into the buffer
    int bitPosition = 0;
    for (int i = 0; i < compressedSize; i++) {
        int code = compressedPayload[i];
        for (int j = 0; j < 12; j++) {
            int byteIndex = (bitPosition / 8);
            int bitIndex = bitPosition % 8;
            packedData[byteIndex] |= ((code >> j) & 1) << bitIndex;
            bitPosition++;
        }
    }

    // Write the size (in terms of number of 12-bit codes) and packed data
    fwrite(&compressedSize, sizeof(compressedSize), 1, file);
    fwrite(packedData, 1, totalBytes, file);

    // Clean up
    free(packedData);
    fclose(file);
    return 0;
}



int main() {
    const char* imageFilename = "../png24.png";
    const char* payloadFilename = "../k.txt";
    const char* outputImageFilename = "../output.png";
    const char* decompressedPayloadFilename = "../decompressed_payload.txt";

    int type = 1; /* bmp=2, png=3 */
    int payloadSize;
    unsigned char* payloadData;
    int* compressedPayload;
    int compressedSize;
    int extractionSuccess;

    /* Step 1: Determine file type and check if it's 24-bit */
    printf("start step1\n");
    type = determineFileTypeAndCheck24Bit(imageFilename);
    if (type == 1) {
        fprintf(stderr, "File is not a 24-bit BMP or PNG.\n");
        return 1;
    }

    /* Step 2: Load payload */
    printf("start step2\n");
    payloadData = readBinaryPayloadData(payloadFilename, &payloadSize);
    if (!payloadData) {
        fprintf(stderr, "Failed to load payload data.\n");
        return 1;
    }


    if(type==2){

        printf("start step3\n");
        compressedPayload = lzwCompress(payloadData, payloadSize, &compressedSize);
        printf("MAIN compressed - %d",compressedSize);
        free(payloadData); // Assume payloadData is no longer needed after this point

        if (!compressedPayload) {
            fprintf(stderr, "Compression failed.\n");
            return 1;
        }

        // Write compressed payload to a file
        printf("I am printing compressed file\n");
        if (writeCompressedPayloadToFile("../compressed_payload.bin", compressedPayload, compressedSize) != 0) {
            fprintf(stderr, "Failed to write compressed payload to file.\n");
            free(compressedPayload);
            return 1;
        }


        // Step 4: Embed payload into image and save the output image
        printf("start step4\n");
        if (embedPayloadInImage(imageFilename, outputImageFilename, compressedPayload, compressedSize, payloadFilename) != 0) {
            fprintf(stderr, "Failed to embed payload into image or create output image.\n");
            free(compressedPayload);
            return 1;
        }
        free(compressedPayload);

        // Step 5: Extract and decompress payload from image
        printf("start step5\n");
        extractionSuccess = extractAndDecompressPayload(outputImageFilename, decompressedPayloadFilename);
        if (extractionSuccess != 0) {
            fprintf(stderr, "Failed to extract and decompress payload from image.\n");
            return 1;
        }



    }


    if(type==3){
        printf("start step4\n");
        printf("Original Playload size in byets: %d\n", payloadSize);
        if (embedPayloadInPNG(imageFilename, outputImageFilename, payloadData, payloadSize) != 0) {
            fprintf(stderr, "Failed to embed payload into image or create output image.\n");
            // Free your payload data if necessary
            return 1;
        }

        // Step 5: Extract and decompress payload from image
        printf("start step5\n");
        extractionSuccess = extractAndDecompressPayloadFromPNG(outputImageFilename, decompressedPayloadFilename);
        if (extractionSuccess != 0) {
            fprintf(stderr, "Failed to extract and decompress payload from image.\n");
            return 1;
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