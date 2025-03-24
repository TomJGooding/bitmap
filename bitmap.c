#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BMP_MAGIC_NUMBER 0x4d42  // "BM" in ASCII
#define BMP_PLANES 1
#define BMP_BIT_COUNT 24
#define BMP_COMPRESSION 0

// Pack header structs to ensure correct byte alignment for file output
#pragma pack(push, 1)

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

#pragma pack(pop)

#define BMP_FILEHEADER_SIZE (sizeof(BMPFileHeader))
#define BMP_INFOHEADER_SIZE (sizeof(BMPInfoHeader))
#define BMP_HEADER_SIZE (BMP_FILEHEADER_SIZE + BMP_INFOHEADER_SIZE)

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

void bitmap_save(Bitmap *bitmap, const char *filename) {
    if (bitmap == NULL || bitmap->pixels == NULL) {
        fprintf(stderr, "ERROR: attempt to save uninitialized bitmap\n");
        exit(EXIT_FAILURE);
    }

    // Each pixel data row must be padded to be a multiple of 4 bytes long
    int row_length = 4 * ((bitmap->width * BMP_BIT_COUNT + 31) / 32);

    // TODO: handle huge size overflow
    int image_size = row_length * bitmap->height;
    int file_size = BMP_HEADER_SIZE + image_size;

    BMPFileHeader file_header = {
        .signature = BMP_MAGIC_NUMBER,
        .file_size = file_size,
        .reserved1 = 0,
        .reserved2 = 0,
        .data_offet = BMP_HEADER_SIZE
    };

    BMPInfoHeader info_header = {
        .size = BMP_INFOHEADER_SIZE,
        .width = bitmap->width,
        .height = bitmap->height,
        .planes = BMP_PLANES,
        .bit_count = BMP_BIT_COUNT,
        .compression = BMP_COMPRESSION,
        .image_size = image_size,
        .x_res = 0,
        .y_res = 0,
        .colors_used = 0,
        .important_colors = 0
    };

    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "ERROR: failed to open file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    if (!fwrite(&file_header, BMP_FILEHEADER_SIZE, 1, file) ||
        !fwrite(&info_header, BMP_INFOHEADER_SIZE, 1, file)) {
        fprintf(stderr, "ERROR: failed to write to file '%s'\n", filename);
        exit(EXIT_FAILURE);
    };

    // TODO: Write the bitmap pixel data
    uint32_t red_pixel = 0xFF0000;
    if (!fwrite(&red_pixel, sizeof(red_pixel), 1, file)) {
        fprintf(stderr, "ERROR: failed to write to file '%s'\n", filename);
        exit(EXIT_FAILURE);
    };

    fclose(file);
}

int main() {
    int width = 1;
    int height = 1;
    Bitmap *bitmap = bitmap_init(width, height);

    char *filename = "out.bmp";
    bitmap_save(bitmap, filename);
    printf("Saved bitmap image to '%s'\n", filename);

    bitmap_free(bitmap);

    return EXIT_SUCCESS;
}
