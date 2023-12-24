#include <png.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
} Pixel;

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
