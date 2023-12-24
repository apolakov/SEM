#include <png.h>
#include <stdlib.h>
#include <stdio.h>
#include "pngs.h"
#include "bmp.h"
#include "lzw.h"
#include <string.h>

int embedPayloadInPNG(const char* imageFilename, const char* outputImageFilename, const int* compressedPayload, int compressedSize) {
    Pixel* imagePixels;
    int imageWidth, imageHeight;

    // Step 1: Read the source PNG image
    if (readPNG(imageFilename, &imagePixels, &imageWidth, &imageHeight) != 0) {
        fprintf(stderr, "Failed to read the PNG file.\n");
        return 1;
    }

    // Calculate the size in bytes, assuming each int represents a single byte
    //mozno * size of int idk  * sizeof(int)
    size_t payloadByteSize = compressedSize * sizeof(int);

    embedSizePNG(imagePixels, payloadByteSize * 8);
    unsigned int testSize = extractSizeFromPixelDataPNG(imagePixels);
    if (testSize != payloadByteSize * 8) {
        fprintf(stderr, "Size embedding test failed! Embedded: %zu, Extracted: %u\n", payloadByteSize * 8, testSize);
        // Handle the error
    }

    // Step 2: Embed the payload into the image
    // Casting from int* to unsigned char* for the function that expects byte data
    if (embedPayloadInPixels(imagePixels, imageWidth, imageHeight, (unsigned char*)compressedPayload, payloadByteSize) != 0) {
        fprintf(stderr, "Failed to embed payload into image.\n");
        free(imagePixels);
        return 1;
    }

    // Step 3: Write the modified image to a new PNG file
    if (writePNG(outputImageFilename, imagePixels, imageWidth, imageHeight) != 0) {
        fprintf(stderr, "Failed to write the output PNG file.\n");
        free(imagePixels);
        return 1;
    }

    // Cleanup
    free(imagePixels);
    return 0; // Success
}


int extractAndDecompressPayloadFromPNG(const char* inputImageFilename, const char* outputPayloadFilename) {
    Pixel* pixels;
    int width, height;

    // Read the PNG image
    if (readPNG(inputImageFilename, &pixels, &width, &height) != 0) {
        fprintf(stderr, "Failed to read PNG file.\n");
        return 1;
    }

    // Extract the compressed payload from the image
    int* compressedPayload;
    int compressedSize;
    if (extractPayloadFromPixels(pixels, width, height, &compressedPayload, &compressedSize) != 0) {
        fprintf(stderr, "Failed to extract payload from image.\n");
        free(pixels);
        return 1;
    }
    free(pixels); // Free the pixel array after extraction

    // Decompress the payload
    int decompressedPayloadSize;
    unsigned char* decompressedPayload = lzwDecompress(compressedPayload, compressedSize, &decompressedPayloadSize);
    free(compressedPayload); // Free the compressed payload array after decompression
    if (!decompressedPayload) {
        fprintf(stderr, "Decompression failed.\n");
        return 1;
    }

    // Save the decompressed payload to a file
    FILE* outputFile = fopen(outputPayloadFilename, "wb");
    if (!outputFile) {
        fprintf(stderr, "Failed to open output file.\n");
        free(decompressedPayload);
        return 1;
    }
    fwrite(decompressedPayload, 1, decompressedPayloadSize, outputFile);
    fclose(outputFile);
    free(decompressedPayload);

    return 0; // Success
}




int readPNG(const char* filename, Pixel** outPixels, int* outWidth, int* outHeight) {


    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Could not open file %s for reading.\n", filename);
        return 1;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        fprintf(stderr, "Could not allocate read struct.\n");
        return 1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        fprintf(stderr, "Could not allocate info struct.\n");
        return 1;
    }


    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        fprintf(stderr, "Error during init_io.\n");
        return 1;
    }


    png_init_io(png_ptr, fp);


    png_read_info(png_ptr, info_ptr);
    printf("readPNG");


    int width = png_get_image_width(png_ptr, info_ptr);
    int height = png_get_image_height(png_ptr, info_ptr);
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    // Ensure the PNG is 24-bit RGB
    if (color_type != PNG_COLOR_TYPE_RGB || bit_depth != 8) {
        fprintf(stderr, "Unsupported image format. Require 24-bit RGB PNG.\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return 1;
    }

    // Allocate memory for each row
    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr, info_ptr));
    }

    png_read_image(png_ptr, row_pointers);

    // Allocate memory for the Pixel array
    Pixel* pixels = (Pixel*) malloc(width * height * sizeof(Pixel));

    for (int y = 0; y < height; y++) {
        png_bytep row = row_pointers[y];
        for (int x = 0; x < width; x++) {
            Pixel* pixel = &pixels[y * width + x];
            png_bytep px = &(row[x * 3]); // 3 bytes per pixel (RGB)
            pixel->blue = px[2];
            pixel->green = px[1];
            pixel->red = px[0];
        }
        free(row_pointers[y]);
    }
    free(row_pointers);

    // Clean up
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);

    // Set output values
    *outPixels = pixels;
    *outWidth = width;
    *outHeight = height;

    return 0; // Success
}

int writePNG(const char* filename, Pixel* pixels, int width, int height) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Could not open file %s for writing.\n", filename);
        return 1;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        fprintf(stderr, "Could not allocate write struct.\n");
        return 1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        fprintf(stderr, "Could not allocate info struct.\n");
        return 1;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        fprintf(stderr, "Error during writing.\n");
        return 1;
    }

    png_init_io(png_ptr, fp);

    // Set the image details
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    // Allocate memory for rows
    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr, info_ptr));
        for (int x = 0; x < width; x++) {
            Pixel* pixel = &pixels[y * width + x];
            png_byte* row = &(row_pointers[y][x * 3]);
            row[0] = pixel->red;
            row[1] = pixel->green;
            row[2] = pixel->blue;
        }
    }

    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    // Cleanup
    for (int y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    return 0; // Success
}

int embedPayloadInPixels(Pixel* pixels, int width, int height, const unsigned char* payload, size_t payloadByteSize) {

    printf("i am trying to embed  %zu",payloadByteSize );
    // Calculate the number of bits available to store the payload
    size_t availableBits = (size_t)width * height; // 1 bit per pixel (only using the blue channel)

    // Calculate the number of bits needed to store the payload
    size_t payloadBits = payloadByteSize * 8; // 8 bits per byte

    // Ensure there is enough space to store the payload
    if (payloadBits > availableBits) {
        fprintf(stderr, "Not enough space in the image for the payload.\n");
        return 1;
    }

    // Embed the payload into the image
    for (size_t i = 0; i < payloadBits; ++i) {
        size_t byteIndex = i / 8;
        size_t bitPosition = i % 8;
        size_t bit = (payload[byteIndex] >> bitPosition) & 1;
        embedBit(&pixels[i], bit);
    }

    return 0; // Success
}


void embedBit(Pixel* pixel, size_t bit) {
    // Clear the least significant bit (LSB) of the blue channel
    pixel->blue &= 0xFE; // 0xFE = 11111110 in binary

    // Set the LSB of the blue channel to the bit we want to embed
    pixel->blue |= (bit & 1);
}


/*
int extractPayloadFromPixels(const Pixel* pixels, int width, int height, unsigned char** outPayload, size_t* outPayloadSize) {
    // Assuming the payload size is stored in the first 32 pixels
    size_t payloadSizeBits = 32;
    size_t payloadSize = 0;
    for (size_t i = 0; i < payloadSizeBits; ++i) {
        size_t bit = (pixels[i].blue & 1); // Extract LSB
        payloadSize |= (bit << i);
    }

    // Allocate memory for the payload
    *outPayloadSize = payloadSize;
    *outPayload = (unsigned char*)malloc(payloadSize);
    if (*outPayload == NULL) {
        fprintf(stderr, "Memory allocation failed for payload extraction.\n");
        return 1;
    }

    // Extract the payload from the image
    for (size_t i = 0; i < payloadSize * 8; ++i) {
        size_t byteIndex = i / 8;
        size_t bitPosition = i % 8;
        size_t pixelIndex = i + payloadSizeBits; // Start after the payload size info
        size_t bit = (pixels[pixelIndex].blue & 1); // Extract LSB
        (*outPayload)[byteIndex] |= (bit << bitPosition);
    }

    return 0; // Success
}

 */

int extractPayloadFromPixels(const Pixel* pixels, int width, int height, int** outCompressedPayload, int* outCompressedSize) {
    // The size of the payload (in bits) is assumed to be stored in the first 32 pixels



    unsigned payloadSizeBits = extractSizeFromPixelDataPNG(pixels);
    size_t payloadSize = payloadSizeBits / 8; // Convert bit count to byte count

    printf("i am extracting %u", payloadSizeBits);
    // Allocate memory for the compressed payload
    *outCompressedPayload = (int*)malloc(payloadSize);
    if (*outCompressedPayload == NULL) {
        fprintf(stderr, "Failed to allocate memory for compressed payload.\n");
        return 1;
    }
    memset(*outCompressedPayload, 0, payloadSize);



    // Extract the payload from the image
    for (unsigned i = 0; i < payloadSizeBits; ++i) {
        int byteIndex = i / 8;
        int bitIndex = i % 8;
        int pixelIndex = 32 + i; // Skip the first 32 pixels used for size storage
        int bit = pixels[pixelIndex].blue & 1; // Get the LSB from the blue channel
        (*outCompressedPayload)[byteIndex] |= (bit << bitIndex); // Store the bit in the payload
    }

    *outCompressedSize = payloadSize / sizeof(int); // Convert byte count to int count
    return 0; // Success
}


void embedSizePNG(Pixel* pixels, unsigned int sizeInBits) {
    printf("Embedding size: %u bits\n", sizeInBits);
    for (int i = 0; i < 32; i++) {
        unsigned int bit = (sizeInBits >> i) & 1;
        embedBit(&pixels[i], bit);
        printf("->Embedding bit %d at index %d\n", bit, i);
    }
}

unsigned int extractSizeFromPixelDataPNG(const Pixel* pixels) {
    unsigned int size = 0;
    for (int i = 0; i < 32; i++) {
        printf("******Pixel %d Blue Channel: %u\n", i, pixels[i].blue);
        unsigned int bit = pixels[i].blue & 1;
        printf("******Extracted bit %d from index %d, Current size: %u\n", bit, i, size);
        size |= (bit << i);
    }
    printf("******Extracted size: %u bits\n", size);
    return size;
}