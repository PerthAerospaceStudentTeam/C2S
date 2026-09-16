#include <stdint.h>
#define USE_DECODE_FUNCTIONS
#define USE_UINT16_FUNCTIONS
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <stdlib.h>
#include "stb_image_resize.h"
#include "stb_image_write.h"
#include "icer.h"
#include "color_util.h"

#define CHUNK_WIDTH 640
#define CHUNK_HEIGHT 480

/*
 * This program is meant to be run on a ground station to decompress the compressed .bin for ICER chunked compression
 */

void yuv_to_rgb888_packed(uint16_t *y_channel, uint16_t *u_channel, uint16_t *v_channel, uint8_t *img, size_t image_w, size_t image_h, size_t rowstride) {
    int32_t y, u, v;
    uint8_t *pixel;

    uint16_t *input_y, *input_u, *input_v;
    for (size_t row = 0;row < image_h;row++) {
        pixel = img + 3 * rowstride * row;
        input_y = y_channel + rowstride * row;
        input_u = u_channel + rowstride * row;
        input_v = v_channel + rowstride * row;
        for (size_t col = 0;col < image_w;col++) {
            y = *input_y;
            u = *input_u;
            v = *input_v;

            pixel[0] = CYCbCr2R(y, u, v);
            pixel[1] = CYCbCr2G(y, u, v);
            pixel[2] = CYCbCr2B(y, u, v);

            pixel += 3;
            input_y++; input_u++; input_v++;
        }
    }
}

#pragma pack(push, 1)
typedef struct {
	uint16_t chunk_dim_x;
	uint16_t chunk_dim_y;
	uint16_t chunk_width;
	uint16_t chunk_height;
    uint16_t original_width;
    uint16_t original_height;
}file_header_t;

#define CHUNK_START_ID 0xA5C3E7F1u

typedef struct {
	uint32_t chunk_start;
	uint16_t chunk_id; // 0 to num_chunks - 1
	uint32_t compressed_size;
}chunk_header_t;
#pragma pack(pop)


int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("USAGE: <compressed.bin> <output name>\n ");
        return 1;
    }

    const char* compress_filename = argv[1];
    const char *output_filename = argv[2];


    // Open compressed file
    FILE* fp = fopen(compress_filename, "rb");

    if (fp == NULL) {
        perror("Failed to open compressed file");
        return 1;
    }


    size_t decomp_w, decomp_h;
    file_header_t file_header;

    // Read file header
    if (fread(&file_header, sizeof(file_header_t), 1, fp) != 1) {
        printf("Failed to read file header\n");
        fclose(fp);
        return 1;
    }

    // Validate file header
    if (file_header.chunk_dim_x == 0 ||
	file_header.chunk_dim_y == 0 ||
        file_header.chunk_width == 0 ||
        file_header.chunk_height == 0) {

        printf("Invalid file header\n");
        fclose(fp);
        return 1;
    }

    printf("File header:\n");
    printf("  chunk_dim_x      : %u\n", file_header.chunk_dim_x);
    printf("  chunk_dim_y      : %u\n", file_header.chunk_dim_y);
    printf("  chunk width  : %u\n", file_header.chunk_width);
    printf("  chunk height : %u\n", file_header.chunk_height);
    printf("  original width  : %u\n", file_header.original_width);
    printf("  original height : %u\n", file_header.original_height);

    size_t chunk_w = file_header.chunk_width;
    size_t chunk_h = file_header.chunk_height;
    uint16_t chunk_dim_x = file_header.chunk_dim_x;
    uint16_t chunk_dim_y = file_header.chunk_dim_y;

    // Full image width and height
    size_t full_w = file_header.original_width;
    size_t full_h = file_header.original_height;

    // Amount of pixels per chunk
    size_t chunk_pixels = chunk_h * chunk_w;

    printf("\nImage:\n");
    printf("  full dimensions  : %zu x %zu\n", full_w, full_h);
    printf("  chunk dimensions : %zu x %zu\n", chunk_w, chunk_h);
    printf("  chunks           : %u x %u = %u\n",
           file_header.chunk_dim_x,
           file_header.chunk_dim_y,
           file_header.chunk_dim_x * file_header.chunk_dim_y);

    icer_init();

    // Allocate full Y/U/V buffers
    // Contains reconstructed, decompressed full image split in Y U V buffers
    uint16_t* decompress[3];

    for (int chan = 0; chan < 3; chan++) {
        
        // Allocate full image size and set bits to 0
        decompress[chan] = calloc(full_w * full_h, sizeof(uint16_t));

        if (decompress[chan] == NULL) {
            printf("Failed to allocate full image buffer\n");

            for (int i = 0; i < chan; i++) {
                free(decompress[i]);
            }

            fclose(fp);
            return 1;
        }
    }

    // Temp chunk buffers to hold chunk Y U and V
    // Used when calling icer_decompress()
    uint16_t* chunk_y = malloc(chunk_pixels * sizeof(uint16_t));
    uint16_t* chunk_u = malloc(chunk_pixels * sizeof(uint16_t));
    uint16_t* chunk_v = malloc(chunk_pixels * sizeof(uint16_t));
    
    if (chunk_y == NULL ||
        chunk_u == NULL ||
        chunk_v == NULL) {

        printf("Failed to allocate chunk buffers\n");

        free(decompress[0]);
        free(decompress[1]);
        free(decompress[2]);

        free(chunk_y);
        free(chunk_u);
        free(chunk_v);

        fclose(fp);

        return 1;
    }

    // Holds compressed chunk from .bin file
    uint8_t* compressed_chunk = NULL;
    size_t compressed_buffer_size = 0;
    size_t num_chunks = chunk_dim_x * chunk_dim_y;
    chunk_header_t chunk_header;

    // Decompress each chunk individually
    for (size_t chunk = 0; chunk < num_chunks; chunk++) {
        printf("\n----------------------------------------\n");
        printf("Reading chunk %zu / %zu\n",
               chunk + 1,
               num_chunks);
        printf("----------------------------------------\n");

        // Read chunk header
        if (fread(&chunk_header, sizeof(chunk_header), 1, fp) != 1) {
            printf("Failed to read chunk header\n");
            goto cleanup;
        }

        // Validate header
        if (chunk_header.chunk_start != CHUNK_START_ID) {
            printf("Invalid chunk start ID\n");
            printf("Expected: 0x%08X\n", CHUNK_START_ID);
            printf("Got     : 0x%08X\n",
                   chunk_header.chunk_start);

            goto cleanup;
        }

        // Validate chunk id
        if (chunk_header.chunk_id >= num_chunks) {
            printf("Invalid chunk ID: %u\n",
                   chunk_header.chunk_id);

            goto cleanup;
        }

        printf("Chunk ID         : %u\n",
               chunk_header.chunk_id);

        printf("Compressed size  : %u bytes\n",
               chunk_header.compressed_size);


        // Determine chunk row and col
        size_t row = chunk_header.chunk_id / chunk_dim_x;
        size_t col = chunk_header.chunk_id % chunk_dim_x;

        // Convert to pixel coordinates
        size_t dst_x = col * chunk_w;
        size_t dst_y = row * chunk_h;
            
        size_t valid_width = full_w - dst_x;

        if (valid_width > chunk_w)
        {
            valid_width = chunk_w;
        }

        size_t valid_height = full_h - dst_y;

        if (valid_height > chunk_h)
        {
            valid_height = chunk_h;
        }

        printf("Chunk position   : row=%zu col=%zu\n",
               row,
               col);

        printf("Destination      : x=%zu y=%zu\n",
               dst_x,
               dst_y);

        size_t compressed_size = chunk_header.compressed_size;

        // Resize compressed buffer if necessary
        if (compressed_size > compressed_buffer_size) {
            // realloc buffer
            uint8_t* new_buffer = realloc(compressed_chunk, compressed_size);

            if (new_buffer == NULL) {

                printf("Failed to allocate "
                       "compressed chunk buffer\n");

                goto cleanup;
            }
            // point compressed_chunk to new_buffer
            compressed_chunk = new_buffer;
            compressed_buffer_size = compressed_size;
        }

        // Read compressed ICER data section
        if (fread(compressed_chunk, 1, compressed_size, fp) != compressed_size) {
            printf("Failed to read compressed chunk data\n");
            goto cleanup;
        }

        size_t decoded_w;
        size_t decoded_h;

        printf("Decoding ICER chunk...\n");

        int res = icer_decompress_image_yuv_uint16(chunk_y, chunk_u, 
                            chunk_v, &decoded_w, 
                            &decoded_h, chunk_pixels, compressed_chunk, 
                            compressed_size);
        
        if (res != ICER_RESULT_OK) {

            printf("ICER decompression failed\n");
            printf("Result: %d\n", res);

            goto cleanup;
        }

        if (decoded_w != chunk_w || decoded_h != chunk_h) {

            printf("ERROR: decoded dimensions do not "
                   "match file header\n");

            goto cleanup;
        } 

        // Copy decoded chunk into y u v image
        for (size_t y = 0; y < valid_height; y++) {

            // position inside the full reconstructed image
            // convert 2d xy coord into a 1d array index
            // row-major indexing
            size_t dst_index = ((dst_y + y) * full_w) + dst_x;

            // position inside the decoded chunk
            size_t src_index = y * chunk_w;

            memcpy(&decompress[0][dst_index],
                &chunk_y[src_index],
                valid_width * sizeof(uint16_t));

            memcpy(&decompress[1][dst_index],
                &chunk_u[src_index],
                valid_width * sizeof(uint16_t));

            memcpy(&decompress[2][dst_index],
                &chunk_v[src_index],
                valid_width * sizeof(uint16_t));
        }

        printf("Chunk copied into full image\n");
    }


    printf("\n========================================\n");
    printf("All chunks decoded successfully\n");
    printf("========================================\n");

    // Create full image buffer
    uint8_t* display = malloc(full_w * full_h * 3);

    if (display == NULL) {
        printf("Failed to allocate RGB buffer\n");
        goto cleanup;
    }

    printf("Converting to rgb\n");

    yuv_to_rgb888_packed(decompress[0], decompress[1], decompress[2], display, full_w, full_h, full_w);

    printf("saving decompressed image to: \"%s\"\n",
           output_filename);

    int res =
        stbi_write_bmp(
            output_filename,
            full_w,
            full_h,
            3,
            display
        );

    if (res == 0) {

        printf("save failed\n");

        free(display);
        goto cleanup;
    }

    printf("Image saved successfully\n");

    free(display);

cleanup:
    
    free(compressed_chunk);
    free(chunk_y);
    free(chunk_u);
    free(chunk_v);

    free(decompress[0]);
    free(decompress[1]);
    free(decompress[2]);

    fclose(fp);

    return 0;

}
