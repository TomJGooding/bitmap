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
    int padding_length = row_length - (bitmap->width * 3);
    uint8_t padding_byte = 0;

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

    // Pixels are stored "bottom-up", starting in the lower left corner
    for (int y = bitmap->height - 1; y >= 0; y--) {
        for (int x = 0; x < bitmap->width; x++) {
            Color pixel_color = bitmap->pixels[y * bitmap->width + x];
            uint8_t red = (pixel_color & 0xFF0000) >> 16;
            uint8_t green = (pixel_color & 0x00FF00) >> 8;
            uint8_t blue = (pixel_color & 0x0000FF);
            // Colors are stored in BGR order
            if (!fwrite(&blue, sizeof(blue), 1, file) ||
                !fwrite(&green, sizeof(green), 1, file) ||
                !fwrite(&red, sizeof(red), 1, file)) {
                fprintf(
                    stderr, "ERROR: failed to write to file '%s'\n", filename
                );
                exit(EXIT_FAILURE);
            };
        }
        for (int i = 0; i < padding_length; i++) {
            if (!fwrite(&padding_byte, sizeof(padding_byte), 1, file)) {
                fprintf(
                    stderr, "ERROR: failed to write to file '%s'\n", filename
                );
                exit(EXIT_FAILURE);
            };
        }
    }

    fclose(file);
}

void bitmap_set_pixel(Bitmap *bitmap, int x, int y, Color color) {
    if (bitmap == NULL || bitmap->pixels == NULL) {
        fprintf(
            stderr, "ERROR: attempt to set pixel in uninitialized bitmap\n"
        );
        exit(EXIT_FAILURE);
    }
    if (x < 0 || y < 0) {
        fprintf(stderr, "ERROR: Pixel coordinates must be non-negative\n");
        exit(EXIT_FAILURE);
    }
    if (x >= bitmap->width || y >= bitmap->height) {
        fprintf(stderr, "ERROR: Pixel coordinate is out of range\n");
        exit(EXIT_FAILURE);
    }

    bitmap->pixels[y * bitmap->width + x] = color;
}

void bitmap_fill_rect(
    Bitmap *bitmap, int x, int y, int width, int height, Color color
) {
    if (bitmap == NULL || bitmap->pixels == NULL) {
        fprintf(
            stderr, "ERROR: attempt to fill pixels in uninitialized bitmap\n"
        );
        exit(EXIT_FAILURE);
    }
    if (x < 0 || y < 0) {
        fprintf(stderr, "ERROR: Starting coordinate must be non-negative\n");
        exit(EXIT_FAILURE);
    }
    if (x >= bitmap->width || y >= bitmap->height) {
        fprintf(stderr, "ERROR: Starting coordinate is out of range\n");
        exit(EXIT_FAILURE);
    }
    // TODO: allow negative width/height values to draw rectangles left/up
    if (width < 0 || height < 0) {
        fprintf(stderr, "ERROR: Rectangle width/height must be non-negative\n");
        exit(EXIT_FAILURE);
    }
    int max_dx = x + width;
    int max_dy = y + height;
    if (max_dx > bitmap->width || max_dy > bitmap->height) {
        fprintf(stderr, "ERROR: Rectangle width/height is out of range\n");
        exit(EXIT_FAILURE);
    }

    for (int dy = y; dy < max_dy; dy++) {
        for (int dx = x; dx < max_dx; dx++) {
            bitmap_set_pixel(bitmap, dx, dy, color);
        }
    }
}

int main() {
    int width = 30;
    int height = 20;
    Bitmap *bitmap = bitmap_init(width, height);

    int band_width = width / 3;
    bitmap_fill_rect(bitmap, 0 * band_width, 0, band_width, height, 0x000091);
    bitmap_fill_rect(bitmap, 1 * band_width, 0, band_width, height, 0xFFFFFF);
    bitmap_fill_rect(bitmap, 2 * band_width, 0, band_width, height, 0xE1000F);

    char *filename = "france.bmp";
    bitmap_save(bitmap, filename);
    printf("Saved bitmap image to '%s'\n", filename);

    bitmap_free(bitmap);

    return EXIT_SUCCESS;
}
