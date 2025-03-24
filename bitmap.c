#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BMP_MAGIC_NUMBER 0x4d42  // "BM" in ASCII
#define BMP_PLANES 1
#define BMP_BIT_COUNT 24
#define BMP_COMPRESSION 0

typedef struct {
    uint16_t signature;   // BMP_MAGIC_NUMBER = "BM"
    uint32_t file_size;   // entire size of file
    uint16_t reserved1;   // must be zero
    uint16_t reserved2;   // must be zero
    uint32_t data_offet;  // offset of pixel data
} BMPFileHeader;

typedef struct {
    uint32_t size;         // this header size (40 bytes)
    int32_t width;         // image width in pixels
    int32_t height;        // image height in pixels
    uint16_t planes;       // must be 1
    uint16_t bit_count;    // bits per pixel
    uint32_t compression;  // compression method
    uint32_t image_size;   // pixel data size

    // unused fields set to zero
    int32_t x_res;
    int32_t y_res;
    uint32_t colors_used;
    uint32_t important_colors;
} BMPInfoHeader;

// The color data should really just be a RGB struct,
// but I want to experiment with bitwise operations!
typedef uint32_t Color;

typedef struct {
    int width;
    int height;
    Color *pixels;
} Bitmap;

Bitmap *bitmap_init(int width, int height) {
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "ERROR: bitmap width/height must be > 0\n");
        exit(EXIT_FAILURE);
    }
    if (width > INT32_MAX || height > INT32_MAX) {
        fprintf(stderr, "ERROR: bitmap width/height is too large\n");
        exit(EXIT_FAILURE);
    }

    Bitmap *bitmap = malloc(sizeof(Bitmap));
    if (bitmap == NULL) {
        fprintf(stderr, "ERROR: out of memory\n");
        exit(EXIT_FAILURE);
    }
    bitmap->width = width;
    bitmap->height = height;

    bitmap->pixels = malloc(sizeof(Color) * width * height);
    if (bitmap->pixels == NULL) {
        fprintf(stderr, "ERROR: out of memory\n");
        exit(EXIT_FAILURE);
    }

    return bitmap;
}

void bitmap_free(Bitmap *bitmap) {
    if (bitmap == NULL || bitmap->pixels == NULL) {
        fprintf(stderr, "ERROR: attempt to free uninitialized bitmap\n");
        exit(EXIT_FAILURE);
    }
    free(bitmap->pixels);
    bitmap->pixels = NULL;
    free(bitmap);
}

int main() {
    int width = 10;
    int height = 10;
    Bitmap *bitmap = bitmap_init(width, height);

    printf("Initialized %dx%d bitmap\n", bitmap->width, bitmap->height);

    bitmap_free(bitmap);

    return EXIT_SUCCESS;
}
