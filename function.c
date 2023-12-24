#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "function.h"
#include <stdlib.h>
//int nextAvailableCode = MAX_CHAR; // Variable to track the next available code in LZW compression
//DictionaryEntry *table[TABLE_SIZE]; // Dictionary for LZW compression
int addToDictionary(const unsigned char *bytes, int length);
//DictionaryEntry* table[TABLE_SIZE];
//int nextAvailableCode = 0;












/*
unsigned char* getBytesFromCode(int code, int *length) {
    if (code >= 0 && code < nextAvailableCode && table[code]) {
        // Set the length to the length of the byte array in the dictionary entry
        *length = table[code]->length;
        // Return a pointer to the byte array
        return table[code]->bytes;

    }
    // If the code is not found, set length to 0 to indicate no data
    *length = 0;

    return NULL;
}
*/


/*

// Function to open the file and determine its type
void check_file_type(const char *filename) {
    FILE *file = fopen(filename, "rb");

    if (!file) {
        perror("File opening failed");
        return;
    }

    if (is_png(file)) {
        printf("%s is a PNG file.\n", filename);
    } else {
        rewind(file);
        if (is_bmp(file)) {
            printf("%s is a BMP file.\n", filename);
        } else {
            printf("%s is neither a PNG nor a BMP file.\n", filename);
        }
    }

    fclose(file);
}
 */

/*

int analyze_file_format(const char *filename) {
    int answer = 0;
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("File opening failed");
        return -1;
    }

    // Check if it's a PNG file
    if (is_png(file)) {
        printf("%s is a PNG file.\n", filename);
        // Now check if it's a 24-bit PNG file
        if (is_24bit_png(file)) {
            printf("%s is a 24-bit RGB PNG file.\n", filename);
            answer = 1;
        } else {
            printf("%s is not a 24-bit RGB PNG file.\n", filename);
            answer = 2;
        }
    }
        // Check if it's a BMP file
    else {
        rewind(file); // Rewind the file pointer for BMP check
        if (is_bmp(file)) {
            printf("%s is a BMP file.\n", filename);
            // Now check if it's a 24-bit BMP file
            if (is_24bit_bmp(file)) {
                printf("%s is a 24-bit RGB BMP file.\n", filename);
                answer = 3;
            } else {
                printf("%s is not a 24-bit RGB BMP file.\n", filename);
                answer = 4;
            }
        } else {
            printf("%s is neither a PNG nor a BMP file.\n", filename);
        }
    }

    fclose(file);
    return answer;
}
 */

/*

void saveDecompressedPayload(const unsigned char* decompressedPayload, int decompressedPayloadSize) {
    if (decompressedPayloadSize < 4) {
        fprintf(stderr, "Decompressed payload is too small to contain a valid file extension.\n");
        return;
    }

    char fileExtension[5];
    strncpy(fileExtension, (const char*)decompressedPayload, 4);
    fileExtension[4] = '\0';

    printf("Extracted file extension: %s\n", fileExtension);

    // Hardcoded filename for testing
    FILE* file = fopen("../test.png", "wb");
    if (!file) {
        perror("Error opening hardcoded file");
        return;
    }


    printf("First few bytes of payload data: ");
    for (int i = 0; i < 15 && i < decompressedPayloadSize - 4; ++i) {
        printf("%02x ", decompressedPayload[i + 4]);
    }
    printf("\n");

    printf("Size of decompressed payload: %d\n", decompressedPayloadSize - 4);


    size_t written = fwrite(decompressedPayload + 4, 1, decompressedPayloadSize - 4, file);
    if (written != decompressedPayloadSize - 4) {
        fprintf(stderr, "Error writing to hardcoded file 'test.png'. Written: %zu, Expected: %d\n", written, decompressedPayloadSize - 4);
    }

    fclose(file);
}
 */



/*
unsigned char* readPayloadData(const char* filename, int* size) {
    FILE* file = fopen(filename, "rb"); // Open the file in binary mode
    if (!file) {
        fprintf(stderr, "Unable to open payload file.\n");
        return NULL;
    }

    // Seek to the end of the file to determine the size
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET); // Seek back to the beginning of the file

    // Allocate memory for the payload data
    unsigned char* data = (unsigned char*)malloc(*size);
    if (!data) {
        fprintf(stderr, "Memory allocation failed for payload data.\n");
        fclose(file);
        return NULL;
    }

    // Read the file into memory
    fread(data, 1, *size, file);
    fclose(file); // Close the file

    return data;
}
 */


//MAIN I AM NOT USING RIGHT NOW
/*
    const char *inputFilename = "../bmp24.bmp";
    const char *outputFilename = "../output.bmp";
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    int pixelDataSize;

    // Analyze the file format to ensure it's a 24-bit BMP file
    int format = analyze_file_format(inputFilename);
    if (format != 3) { // If the file is not a 24-bit RGB BMP file
        fprintf(stderr, "The file is not a 24-bit RGB BMP file.\n");
        return 1;
    }

    // Open the file for reading pixel data
    FILE *inputFile = fopen(inputFilename, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open BMP file.\n");
        return 1;
    }
    // Read the headers (they have already been checked by analyze_file_format)
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, inputFile);
    fread(&bih, sizeof(BITMAPINFOHEADER), 1, inputFile);

    unsigned char *pixelData = readPixelData(inputFile, bfh, bih, &pixelDataSize);
    fclose(inputFile); // Close the file after reading pixel data

    if (!pixelData) {
        fprintf(stderr, "Failed to read pixel data from BMP file.\n");
        return 1;
    }

    const char* payloadFilename = "../png.png";

    unsigned char* payloadData;
    int payloadSize;
    payloadData = readPayloadData(payloadFilename, &payloadSize);


    int compressedSize = payloadSize; // Initialize to payload size, will be updated by lzwCompress
    int* compressedPayload = lzwCompress(payloadData, &compressedSize);

    FILE* compressedPayloadFile = fopen("../compressed_payload.bin", "wb");
    if (!compressedPayloadFile) {
        perror("Failed to open file to save compressed payload");
        return 1;
    }
    fwrite(compressedPayload, sizeof(int), compressedSize, compressedPayloadFile);
    fclose(compressedPayloadFile);

    //printLZWCompressedData(* compressedPayload, 10);
    free(payloadData); // Free the original payload data as it is no longer needed

    if (pixelDataSize < compressedSize * 8) {
        fprintf(stderr, "Not enough space in the image for the payload.\n");
        return 3;
    }

    embedPayload(pixelData, pixelDataSize, compressedPayload, compressedSize);
    free(compressedPayload); // Free the compressed payload as it has been embedded

    saveImage(outputFilename, bfh, bih, pixelData, pixelDataSize);
    free(pixelData);

    FILE *modifiedFile = fopen(outputFilename, "rb");
    if (!modifiedFile) {
        fprintf(stderr, "Failed to open the modified BMP file.\n");
        return 1;
    }
    // Read the headers again from the modified file
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, modifiedFile);
    fread(&bih, sizeof(BITMAPINFOHEADER), 1, modifiedFile);

// Read the pixel data from the modified file
    unsigned char *modifiedPixelData = readPixelData(modifiedFile, bfh, bih, &pixelDataSize);
    fclose(modifiedFile);

    if (!modifiedPixelData) {
        fprintf(stderr, "Failed to read pixel data from the modified BMP file.\n");
        return 1;
    }

    int compressedPayloadSize;
    int *extractedCompressedPayload = extractPayload(modifiedPixelData, pixelDataSize, &compressedPayloadSize);

    free(modifiedPixelData);  // Free the modified pixel data after extracting the payload

    int decompressedPayloadSize;
    unsigned char* decompressedPayload = lzwDecompress(extractedCompressedPayload, compressedPayloadSize);


    saveDecompressedPayload(decompressedPayload, decompressedPayloadSize);

    // Save the decompressed payload to a file
    FILE* decompressedPayloadFile = fopen("../decompressed_payload.bin", "wb");
    if (!decompressedPayloadFile) {
        perror("Failed to open file to save decompressed payload");
        return 1;
    }
    fwrite(decompressedPayload, 1, decompressedPayloadSize, decompressedPayloadFile);
    fclose(decompressedPayloadFile);

    if (compareFiles("../compressed_payload.bin", "../decompressed_payload.bin") == 0) {
        printf("Success: The embedded and extracted payloads are identical.\n");
    } else {
        printf("Error: The payloads do not match!\n");
    }

    // Clean up
    free(extractedCompressedPayload);
    */


/*
int addToDictionary(const unsigned char *bytes, int length) {
    if (nextAvailableCode >= TABLE_SIZE) {
        fprintf(stderr, "Dictionary is full.\n");
        return -1;
    }

    table[nextAvailableCode] = (DictionaryEntry *)malloc(sizeof(DictionaryEntry));
    if (!table[nextAvailableCode]) {
        fprintf(stderr, "Memory allocation failed.\n");
        return -1;
    }

    table[nextAvailableCode]->bytes = (unsigned char *)malloc(length);
    if (!table[nextAvailableCode]->bytes) {
        free(table[nextAvailableCode]);
        table[nextAvailableCode] = NULL;
        fprintf(stderr, "Memory allocation failed for dictionary bytes.\n");
        return -1;
    }

    memcpy(table[nextAvailableCode]->bytes, bytes, length);
    table[nextAvailableCode]->length = length;
    table[nextAvailableCode]->code = nextAvailableCode;
    nextAvailableCode++;

    return 0;
}
 */

