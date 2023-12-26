#include <png.h>
#include <stdlib.h>
#include <stdio.h>
#include "pngs.h"
#include "bmp.h"
#include "lzw.h"
#include <string.h>

int embedPayloadInPNG(const char* imageFilename, const char* outputImageFilename, const unsigned char* originalPayload, int originalPayloadSize) {
    if (!imageFilename || !outputImageFilename || !originalPayload) {
        fprintf(stderr, "Null pointer passed to parameters.\n");
        return 1;
    }

    // Compress the payload using LZW
    int compressedPayloadSize;
    int* compressedPayload = lzwCompress(originalPayload, originalPayloadSize, &compressedPayloadSize);
    if (!compressedPayload) {
        fprintf(stderr, "LZW compression failed.\n");
        return 1;
    }

    Pixel* imagePixels;
    int imageWidth, imageHeight;

    printf("Reading the source PNG image...\n");
    if (readPNG(imageFilename, &imagePixels, &imageWidth, &imageHeight) != 0) {
        fprintf(stderr, "Failed to read the PNG file: %s\n", imageFilename);
        free(compressedPayload); // Clean up compressed payload
        return 1;
    }

    printf("Calculating the compressed payload byte size...\n");
    size_t payloadByteSize = compressedPayloadSize * sizeof(int); // Compressed payload size in bytes
    size_t payloadBitSize = payloadByteSize * 8; // Compressed payload size in bits
    printf("Compressed payload byte size: %zu, Payload bit size: %zu\n", payloadByteSize, payloadBitSize);

    printf("Embedding the payload size into the image...\n");
    embedSizePNG(imagePixels, payloadBitSize);
    unsigned int testSize = extractSizeFromPixelDataPNG(imagePixels);
    if (testSize != payloadBitSize) {
        fprintf(stderr, "Size embedding test failed! Embedded: %zu, Extracted: %u\n", payloadBitSize, testSize);
        free(imagePixels);
        free(compressedPayload);
        return 1;
    }

    printf("Embedding compressed payload into image...\n");
    if (embedPayloadInPixels(imagePixels, imageWidth, imageHeight, (unsigned char*)compressedPayload, payloadByteSize) != 0) {
        fprintf(stderr, "Failed to embed payload into image.\n");
        free(imagePixels);
        free(compressedPayload);
        return 1;
    }

    printf("Writing the modified image to a new PNG file...\n");
    if (writePNG(outputImageFilename, imagePixels, imageWidth, imageHeight) != 0) {
        fprintf(stderr, "Failed to write the output PNG file: %s\n", outputImageFilename);
        free(imagePixels);
        free(compressedPayload);
        return 1;
    }

    printf("Image written successfully to %s\n", outputImageFilename);

    free(imagePixels);
    free(compressedPayload);
    printf("Memory cleaned up and function completed successfully.\n");
    return 0; // Success
}


int extractAndDecompressPayloadFromPNG(const char* inputImageFilename, const char* outputPayloadFilename) {
    if (!inputImageFilename || !outputPayloadFilename) {
        fprintf(stderr, "Null pointer passed to parameters.\n");
        return 1;
    }

    Pixel* pixels;
    int width, height;

    printf("Reading the PNG image: %s\n", inputImageFilename);
    if (readPNG(inputImageFilename, &pixels, &width, &height) != 0) {
        fprintf(stderr, "Failed to read PNG file: %s\n", inputImageFilename);
        return 1;
    }

    int* compressedPayload;
    int compressedSizeBits;
    printf("Extracting the compressed payload from the image...\n");
    if (extractPayloadFromPixels(pixels, width, height, &compressedPayload, &compressedSizeBits) != 0) {
        fprintf(stderr, "Failed to extract payload from image.\n");
        free(pixels);
        return 1;
    }

    int compressedSizeBytes = compressedSizeBits / 8;
    printf("Decompressing the payload. Size (bytes): %d\n", compressedSizeBytes);
    int decompressedPayloadSize;
    unsigned char* decompressedPayload = lzwDecompress(compressedPayload, compressedSizeBytes / sizeof(int), &decompressedPayloadSize);    free(compressedPayload);
    if (!decompressedPayload) {
        fprintf(stderr, "Decompression failed.\n");
        free(pixels);
        return 1;
    }

    printf("Saving the decompressed payload to file: %s\n", outputPayloadFilename);
    FILE *outputFile;
    fopen_s(&outputFile, outputPayloadFilename, "wb");
    if (!outputFile) {
        fprintf(stderr, "Failed to open output file: %s\n", outputPayloadFilename);
        free(decompressedPayload);
        free(pixels);
        return 1;
    }
    fwrite(decompressedPayload, 1, decompressedPayloadSize, outputFile);
    fclose(outputFile);

    free(decompressedPayload);
    free(pixels);
    printf("Decompressed payload saved successfully.\n");

    return 0; // Success
}






int readPNG(const char* filename, Pixel** outPixels, int* outWidth, int* outHeight) {
    if (!filename || !outPixels || !outWidth || !outHeight) {
        fprintf(stderr, "Null pointer passed to readPNG.\n");
        return 1;
    }

    FILE *fp;
    fopen_s(&fp, filename, "rb");
    if (!fp) {
        fprintf(stderr, "Could not open file %s for reading.\n", filename);
        return 1;
    }

    printf("Initializing PNG read structure for file: %s\n", filename);
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

    png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
    png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    printf("Reading PNG with Width: %u, Height: %u\n", width, height);

    if (color_type != PNG_COLOR_TYPE_RGB || bit_depth != 8) {
        fprintf(stderr, "Unsupported image format. Require 24-bit RGB PNG.\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return 1;
    }

    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    if (!row_pointers) {
        fprintf(stderr, "Memory allocation failed for row pointers.\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return 1;
    }

    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr, info_ptr));
        if (!row_pointers[y]) {
            fprintf(stderr, "Memory allocation failed for row %d.\n", y);
            // Free previously allocated rows
            for (int j = 0; j < y; j++) {
                free(row_pointers[j]);
            }
            free(row_pointers);
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            fclose(fp);
            return 1;
        }
    }

    png_read_image(png_ptr, row_pointers);

    Pixel* pixels = (Pixel*) malloc(width * height * sizeof(Pixel));
    if (!pixels) {
        fprintf(stderr, "Memory allocation failed for pixel data.\n");
        for (int y = 0; y < height; y++) {
            free(row_pointers[y]);
        }
        free(row_pointers);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return 1;
    }

    for (int y = 0; y < height; y++) {
        png_bytep row = row_pointers[y];
        for (int x = 0; x < width; x++) {
            Pixel* pixel = &pixels[y * width + x];
            png_bytep px = &(row[x * 3]);
            pixel->blue = px[2];
            pixel->green = px[1];
            pixel->red = px[0];
        }
        free(row_pointers[y]);
    }
    free(row_pointers);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);

    *outPixels = pixels;
    *outWidth = width;
    *outHeight = height;

    printf("PNG read successfully.\n");
    return 0;
}


int writePNG(const char* filename, Pixel* pixels, int width, int height) {
    if (!filename || !pixels) {
        fprintf(stderr, "Null pointer passed to writePNG.\n");
        return 1;
    }

    FILE *fp;
    fopen_s(&fp, filename, "wb");
    if (!fp) {
        fprintf(stderr, "Could not open file %s for writing.\n", filename);
        return 1;
    }

    printf("Initializing PNG write structure for file: %s\n", filename);
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
        fprintf(stderr, "Error during writing initialization.\n");
        return 1;
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    if (!row_pointers) {
        fprintf(stderr, "Memory allocation failed for row pointers.\n");
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return 1;
    }

    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr, info_ptr));
        if (!row_pointers[y]) {
            fprintf(stderr, "Memory allocation failed for row %d.\n", y);
            // Free previously allocated rows
            for (int j = 0; j < y; j++) {
                free(row_pointers[j]);
            }
            free(row_pointers);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            fclose(fp);
            return 1;
        }

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

    for (int y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    printf("PNG written successfully to %s.\n", filename);
    return 0;
}




int embedPayloadInPixels(Pixel* pixels, int width, int height, const unsigned char* payload, size_t payloadByteSize) {
    if (!pixels || !payload) {
        fprintf(stderr, "Null pointer passed to embedPayloadInPixels.\n");
        return 1;
    }

    size_t payloadBits = payloadByteSize * 8;
    size_t imageCapacity = (size_t)width * height;

    // Debug: Print payload size
    printf("Embedding payload: Byte size: %zu, Bit size: %zu image capacity is %zu\n", payloadByteSize, payloadBits, imageCapacity);

    if (payloadBits + 32 > imageCapacity) {
        fprintf(stderr, "Image does not have enough capacity for the payload. Available: %zu, Required: %zu\n", imageCapacity, payloadBits + 32);
        return 1;
    }

    printf("Embedding payload: %zu bits into an image of capacity: %zu pixels.\n", payloadBits, imageCapacity);

    for (size_t i = 0; i < payloadBits; ++i) {
        size_t byteIndex = i / 8;
        size_t bitIndex = 7 - (i % 8);
        size_t pixelIndex = i + 32; // Skip the first 32 pixels for the size storage

        if (pixelIndex >= imageCapacity) {
            fprintf(stderr, "Pixel index out of bounds. Index: %zu, Capacity: %zu\n", pixelIndex, imageCapacity);
            return 1;
        }

        unsigned char bit = (payload[byteIndex] >> bitIndex) & 1;
        pixels[pixelIndex].blue = (pixels[pixelIndex].blue & 0xFE) | bit;

        //printf("Embedding bit %u at pixel index %zu.\n", bit, pixelIndex);
    }
    printf("Embedding payload: Byte size: %zu, Bit size: %zu image capacity is %zu\n", payloadByteSize, payloadBits, imageCapacity);


    printf("Payload embedding completed successfully.\n");
    return 0;
}




// Embedding a bit in the blue channel
void embedBit(Pixel* pixel, size_t bit) {
    pixel->blue = (pixel->blue & 0xFE) | (bit & 1);
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

int extractPayloadFromPixels(const Pixel* pixels, int width, int height, unsigned char** outPayload, size_t* outPayloadSizeBits) {
    if (!pixels || !outPayload || !outPayloadSizeBits) {
        fprintf(stderr, "Null pointer passed to extractPayloadFromPixels.\n");
        return 1;
    }

    unsigned int payloadSizeBits = extractSizeFromPixelDataPNG(pixels);
    size_t payloadByteSize = (payloadSizeBits + 7) / 8; // Convert bits to bytes
    size_t totalPixels = (size_t)width * height;

    printf("Extracting payload... Size (bits): %u, Total Pixels: %zu\n", payloadSizeBits, totalPixels);

    if (payloadSizeBits + 32 > totalPixels * 8) {
        fprintf(stderr, "Payload size exceeds image capacity.\n");
        return 1;
    }

    *outPayload = (unsigned char*)malloc(payloadByteSize);
    if (*outPayload == NULL) {
        fprintf(stderr, "Failed to allocate memory for payload extraction.\n");
        return 1;
    }

    memset(*outPayload, 0, payloadByteSize);
    size_t startPixelIndex = 32; // Start after the size pixels


    for (size_t i = 0; i < payloadSizeBits; ++i) {
        size_t byteIndex = i / 8;
        size_t bitIndex = 7 - (i % 8);
        size_t pixelIndex = startPixelIndex + i;

        if (pixelIndex >= totalPixels) {
            fprintf(stderr, "Pixel index out of bounds. Index: %zu, Total Pixels: %zu\n", pixelIndex, totalPixels);
            free(*outPayload);
            *outPayload = NULL;
            return 1;
        }

        unsigned char bit = pixels[pixelIndex].blue & 1;
        (*outPayload)[byteIndex] |= (bit << bitIndex);
        //printf("Extracting bit %u from pixel index %zu.\n", bit, pixelIndex);
    }

    *outPayloadSizeBits = payloadSizeBits;
    printf("Payload extraction completed successfully.\n");
    return 0;
}




void embedSizePNG(Pixel* pixels, unsigned int sizeInBits) {
    printf("\n\nI AM HERE\n\n");
    if (!pixels) {
        fprintf(stderr, "Null pointer passed to embedSizePNG.\n");
        return;
    }

    printf("Embedding payload size in bits: %u\n", sizeInBits);
    for (int i = 0; i < 32; ++i) {
        unsigned int bit = (sizeInBits >> (31 - i)) & 1; // Extract bits from MSB to LSB
        pixels[i].blue = (pixels[i].blue & 0xFE) | bit; // Embed the bit in the LSB
        printf("SIZE->Embedding bit %u at pixel index %d, resulting blue value: %d\n", bit, i, pixels[i].blue);
    }
    printf("I am trying to embed size %d Bits",sizeInBits);
}


unsigned int extractSizeFromPixelDataPNG(const Pixel* pixels) {
    if (!pixels) {
        fprintf(stderr, "Null pointer passed to extractSizeFromPixelDataPNG.\n");
        return 0;
    }

    unsigned int size = 0;
    printf("Extracting payload size from image...\n");
    for (int i = 0; i < 32; ++i) {
        unsigned int bit = extractBit(&pixels[i]);
        size |= (bit << (31 - i)); // Correct bit position
        //printf("Extracted bit %u from pixel index %d, resulting size so far: %u\n", bit, i, size);
    }
    printf("Extracted payload size in bits: %u\n", size);
    return size;
}



unsigned int extractBit(const Pixel* pixel) {
    unsigned int bit = pixel->blue & 1;
    //printf("Extracted bit %u from blue value %d\n", bit, pixel->blue);
    return bit;
}