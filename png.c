#include <png.h>
#include <stdlib.h>
#include <stdio.h>
#include "pngs.h"
#include "bmp.h"
#include "lzw.h"
#include <string.h>
#include "checksuma.h"


int embedPayloadInPNG(const char* imageFilename, const char* outputImageFilename, const unsigned char* originalPayload, int originalPayloadSize) {
    int compressedPayloadSize, compressedPayloadBitSize;
    int* compressedPayload;
    unsigned long crc;
    Pixel* imagePixels;
    int imageWidth, imageHeight;
    size_t payloadByteSize, payloadBitSize;
    unsigned int testSize;

    if (!imageFilename || !outputImageFilename || !originalPayload) {
        fprintf(stderr, "Null pointer passed to parameters.\n");
        return 1;
    }

    compressedPayload = lzwCompress(originalPayload, originalPayloadSize, &compressedPayloadSize);
    if (!compressedPayload) {
        fprintf(stderr, "LZW compression failed.\n");
        return 1;
    }

    compressedPayloadBitSize = compressedPayloadSize;
    crc = calculateCRC32FromBits(compressedPayload, compressedPayloadBitSize);

    if (readPNG(imageFilename, &imagePixels, &imageWidth, &imageHeight) != 0) {
        fprintf(stderr, "Failed to read the PNG file: %s\n", imageFilename);
        free(compressedPayload);
        return 1;
    }

    embedSignature(imagePixels);

    payloadByteSize = (compressedPayloadSize * 12 + 7) / 8;
    payloadBitSize = compressedPayloadSize * 12;
    embedSizePNG(imagePixels, payloadBitSize);

    testSize = extractSizeFromPixelDataPNG(imagePixels);
    if (testSize != payloadBitSize) {
        fprintf(stderr, "Size embedding test failed! Embedded: %zu, Extracted: %u\n", payloadBitSize, testSize);
        free(imagePixels);
        free(compressedPayload);
        return 1;
    }

    if (embedPayloadInPixels(imagePixels, imageWidth, imageHeight, (unsigned char*)compressedPayload, payloadByteSize) != 0) {
        fprintf(stderr, "Failed to embed payload into image.\n");
        free(imagePixels);
        free(compressedPayload);
        return 1;
    }

    embedCRCInPixels(imagePixels, imageWidth, imageHeight, crc, payloadBitSize);

    if (writePNG(outputImageFilename, imagePixels, imageWidth, imageHeight) != 0) {
        fprintf(stderr, "Failed to write the output PNG file: %s\n", outputImageFilename);
        free(imagePixels);
        free(compressedPayload);
        return 1;
    }

    free(imagePixels);
    free(compressedPayload);
    printf("Memory cleaned up and function completed successfully.\n");
    return 0;
}


int extractAndDecompressPayloadFromPNG(const char* inputImageFilename, const char* outputPayloadFilename) {
    Pixel* pixels;
    int width, height, compressedSizeBits, decompressedPayloadSize, compressedSizeBytes;
    int* compressedPayload;
    unsigned long extractedCRC, calculatedCRC;
    unsigned char* decompressedPayload;
    FILE *outputFile;

    if (!inputImageFilename || !outputPayloadFilename) {
        fprintf(stderr, "Null pointer passed to parameters.\n");
        return 1;
    }

    if (readPNG(inputImageFilename, &pixels, &width, &height) != 0) {
        fprintf(stderr, "Failed to read PNG file: %s\n", inputImageFilename);
        return 1;
    }

    if (!extractAndCheckSignature(pixels)) {
        fprintf(stderr, "Signature mismatch or not found in PNG.\n");
        free(pixels);
        return 1;
    }

    if (extractPayloadFromPixels(pixels, width, height, &compressedPayload, &compressedSizeBits) != 0) {
        fprintf(stderr, "Failed to extract payload from image.\n");
        free(pixels);
        return 1;
    }

    extractedCRC = extractCRCFromPixels(pixels, width, height, compressedSizeBits * 12);
    calculatedCRC = calculateCRC32FromBits(compressedPayload, compressedSizeBits);

    if (extractedCRC != calculatedCRC) {
        fprintf(stderr, "CRC mismatch. Payload may be corrupted.\n");
        free(compressedPayload);
        free(pixels);
        return 1;
    }

    compressedSizeBytes = compressedSizeBits / 8;
    decompressedPayload = lzwDecompress(compressedPayload, compressedSizeBits, &decompressedPayloadSize);
    if (!decompressedPayload) {
        fprintf(stderr, "Decompression failed.\n");
        free(pixels);
        return 1;
    }

    outputFile = fopen(outputPayloadFilename, "wb");
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
    return 0;
}




int readPNG(const char* filename, Pixel** outPixels, int* outWidth, int* outHeight) {
    if (!filename || !outPixels || !outWidth || !outHeight) {
        fprintf(stderr, "Null pointer passed to readPNG.\n");
        return 1;
    }

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

    return 0;
}


int writePNG(const char* filename, Pixel* pixels, int width, int height) {
    if (!filename || !pixels) {
        fprintf(stderr, "Null pointer passed to writePNG.\n");
        return 1;
    }

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
        fprintf(stderr, "Error during writing initialization.\n");
        return 1;
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    if (!row_pointers) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        fprintf(stderr, "Memory allocation failed for row pointers.\n");
        return 1;
    }

    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr, info_ptr));
        if (!row_pointers[y]) {
            for (int j = 0; j < y; j++) {
                free(row_pointers[j]);
            }
            free(row_pointers);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            fclose(fp);
            fprintf(stderr, "Memory allocation failed for row %d.\n", y);
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




int embedPayloadInPixels(Pixel* pixels, int width, int height, const int* compressedPayload, int compressedPayloadSize) {
    if (!pixels || !compressedPayload) {
        fprintf(stderr, "Null pointer passed to embedPayloadInPixels.\n");
        return 1;
    }

    size_t payloadBits, imageCapacity, startPixel, bitPosition, pixelIndex;
    unsigned char bit;
    int i, j;

    printf("compressedpayloadsizze %d\n", compressedPayloadSize);
    payloadBits = compressedPayloadSize * 12; // 12 bits per code
    imageCapacity = (size_t)width * height;
    startPixel = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS; // Adjust for signature and size field

    if (payloadBits + startPixel > imageCapacity) {
        fprintf(stderr, "Image does not have enough capacity for the payload. Available: %zu, Required: %zu\n", imageCapacity, payloadBits + 32);
        return 1;
    }

    bitPosition = 0;
    for (i = 0; i < compressedPayloadSize; ++i) {
        for (j = 0; j < 12; ++j) {
            pixelIndex = bitPosition + startPixel;
            if (pixelIndex >= imageCapacity) {
                fprintf(stderr, "Pixel index out of bounds. Index: %zu, Capacity: %zu\n", pixelIndex, imageCapacity);
                return 1;
            }
            bit = (compressedPayload[i] >> j) & 1;
            embedBit(&pixels[pixelIndex], bit);
            bitPosition++;
        }
    }
    return 0;
}


// Embedding a bit in the blue channel
void embedBit(Pixel* pixel, size_t bit) {
    pixel->blue = (pixel->blue & 0xFE) | (bit & 1);
}



int extractPayloadFromPixels(const Pixel* pixels, int width, int height, int** outCompressedPayload, int* outCompressedPayloadSize) {
    if (!pixels || !outCompressedPayload || !outCompressedPayloadSize) {
        fprintf(stderr, "Null pointer passed to extractPayloadFromPixels.\n");
        return 1;
    }

    size_t startPixel, totalPixels, bitPosition, i, codeIndex, bitIndexInCode, pixelIndex;
    unsigned int payloadSizeBits;
    unsigned char bit;

    startPixel = SIGNATURE_SIZE_BITS + SIZE_FIELD_BITS; // Adjust for signature and size field
    payloadSizeBits = extractSizeFromPixelDataPNG(pixels);
    *outCompressedPayloadSize = (payloadSizeBits + 11) / 12;
    totalPixels = (size_t)width * height;

    if (payloadSizeBits + startPixel > totalPixels * 8) {
        fprintf(stderr, "Payload size exceeds image capacity.\n");
        return 1;
    }

    *outCompressedPayload = (int*)malloc(*outCompressedPayloadSize * sizeof(int));
    if (*outCompressedPayload == NULL) {
        fprintf(stderr, "Failed to allocate memory for compressed payload extraction.\n");
        return 1;
    }

    memset(*outCompressedPayload, 0, *outCompressedPayloadSize * sizeof(int));
    bitPosition = 0;
    for (i = 0; i < payloadSizeBits; ++i) {
        codeIndex = i / 12;
        bitIndexInCode = i % 12;
        pixelIndex = i + startPixel;

        if (pixelIndex >= totalPixels) {
            fprintf(stderr, "Pixel index out of bounds. Index: %zu, Total Pixels: %zu\n", pixelIndex, totalPixels);
            free(*outCompressedPayload);
            *outCompressedPayload = NULL;
            return 1;
        }

        bit = extractBit(&pixels[pixelIndex]);
        (*outCompressedPayload)[codeIndex] |= (bit << bitIndexInCode);
    }

    return 0;
}



void embedSizePNG(Pixel* pixels, unsigned int sizeInBits) {
    if (!pixels) {
        fprintf(stderr, "Null pointer passed to embedSizePNG.\n");
        return;
    }

    int startIndex = SIGNATURE_SIZE_BITS;
    printf("Embedding payload size in bits: %u\n", sizeInBits);
    for (int i = 0; i < 32; ++i) {
        unsigned int bit = (sizeInBits >> (31 - i)) & 1;
        pixels[startIndex + i].blue = (pixels[startIndex + i].blue & 0xFE) | bit;
    }
}


unsigned int extractBit(const Pixel* pixel) {
    if (!pixel) {
        fprintf(stderr, "Null pointer passed to extractBit.\n");
        return 0;
    }

    unsigned int bit = pixel->blue & 1;
    return bit;
}

unsigned int extractSizeFromPixelDataPNG(const Pixel* pixels) {
    if (!pixels) {
        fprintf(stderr, "Null pointer passed to extractSizeFromPixelDataPNG.\n");
        return 0;
    }

    int startIndex = SIGNATURE_SIZE_BITS; // Start index after the signature
    unsigned int size = 0;

    for (int i = 0; i < 32; ++i) {
        unsigned int bit = extractBit(&pixels[startIndex + i]);
        size |= (bit << (31 - i));
    }
    return size;
}
