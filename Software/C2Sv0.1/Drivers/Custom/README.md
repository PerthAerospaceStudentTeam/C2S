# ICER Compression

## Overview

This custom driver uses ICER compression to compress image data using stored Y U V data on the SD card.

The Y, U and V image planes are stored as seperate binary files. Due to limited RAM, the image must be processed in
chunks. This current implementation uses fixed-size 640x480 chunks.

## Issues

Currently, there is a performance bug with the chunking implementation. Exact cause is still unknown however it can be triggered by
reducing the chunk size below 640x480 and if the **ICER_BYTE_QUOTA** becomes to small. Future work on this would require to profile individual ICER functions
to determine exactly where the slow down is occurring.

## Core Variables

| Variable                   | Description                                                                                |
| -------------------------- | ------------------------------------------------------------------------------------------ |
| `SOURCE_IMG_WIDTH`         | Width of the source image in pixels.                                                       |
| `SOURCE_IMG_HEIGHT`        | Height of the source image in pixels.                                                      |
| `CHUNK_WIDTH`              | Fixed width of each image chunk in pixels.                                                 |
| `CHUNK_HEIGHT`             | Fixed height of each image chunk in pixels.                                                |
| `MAX_CHUNK_PIXELS`         | Maximum number of pixels that can be stored in a single chunk buffer.                      |
| `chunks_x`                 | Number of chunks required across the image width.                                          |
| `chunks_y`                 | Number of chunks required across the image height.                                         |
| `num_chunks`               | Total number of chunks in the image (`chunks_x × chunks_y`).                               |
| `chunk_id`                 | Unique sequential identifier assigned to each chunk.                                       |
| `Y`                        | Buffer containing the Y (luma) data for the current chunk.                                 |
| `U`                        | Buffer containing the U (chroma) data for the current chunk.                               |
| `V`                        | Buffer containing the V (chroma) data for the current chunk.                               |
| `compressed_output`        | Buffer used to store ICER's compressed output for the current chunk.                       |
| `chunk_compression_cycles` | Stores the CPU cycle count required to compress each chunk.                                |
| `benchmark_clock`          | Stores the CPU clock frequency used for each benchmark.                                    |
| `benchmark_cycles`         | Stores the total ICER compression cycles for each clock configuration.                     |
| `benchmark_time_us`        | Stores the calculated total compression time in microseconds for each clock configuration. |
| `stages`                   | Number of ICER compression stages used during compression.                                 |
| `filt`                     | ICER filter type used during compression.                                                  |
| `segments`                 | Number of ICER segments used during compression.                                           |
| `ICER_BYTE_QUOTA`          | Maximum compressed output size allowed for each chunk.                                     |

## Core Methods

`uint32_t Perform_ICER_Compress_From_SD()` Call this in main to use the library
`static uint32_t Write_Chunks_To_SD(uint16_t num_chunks)` For debugging, writes chunk cycles data to SD card
`static uint32_t Write_Benchmark_To_SD()` Writes prescalar data to SD card for profiling and debugging
`static uint32_t ICER_Compress_From_SD(...)` performs the complete chunk-based ICER compression process for a YUV image stored on the SD card. Loops through all of x and y chunks
### Processing Flow

For each chunk:

1. **Reset input and output buffers**
   - The Y, U, V and compressed-output buffers are cleared with `memset()`.
   - This ensures that data from the previous chunk cannot remain in the buffers.

2. **Initialise the ICER output structure**
   - `icer_init_output_struct()` is called for every chunk.
   - The compressed output buffer and configured `ICER_BYTE_QUOTA` are supplied to ICER.

3. **Load the YUV chunk**
   - `Load_Data_Chunk_From_SD()` loads the corresponding region from each Y, U and V file.
   - Three separate buffers are used:
     - `Y[MAX_CHUNK_PIXELS]`
     - `U[MAX_CHUNK_PIXELS]`
     - `V[MAX_CHUNK_PIXELS]`
   - Edge chunks are zero-padded when they do not contain a complete `CHUNK_WIDTH × CHUNK_HEIGHT` region.

4. **Compress the chunk**
   icer_compress_image_yuv_uint16()

`static uint32_t Load_Data_Chunk_From_SD(...)` Loads data from the SD card for one chunk. Uses pixel based indexing to determine where to start and stop

## Basic Usage
1. **Configure the source image**
	Set the source image dimensions using `SOURCE_IMG_WIDTH` and `SOURCE_IMG_HEIGHT`:
	
	#define SOURCE_IMG_WIDTH  1920
	#define SOURCE_IMG_HEIGHT 1080
	
	The dimensions must match the dimensions of the YUV input files stored on the SD card.
	
	Configure the corresponding Y, U and V filenames:
	
	static char* filenames[] = {
	    "Y_1920x1080.bin",
	    "U_1920x1080.bin",
	    "V_1920x1080.bin"
	};
	
	The files are expected to contain the Y, U and V planes as separate binary files containing uint16_t pixel data in row-major order.
	
2. **Configure Chunk Dimensions**

	Set the dimensions of each chunk:
	
	#define CHUNK_WIDTH  640U
	#define CHUNK_HEIGHT 480U
	#define MAX_CHUNK_PIXELS 307200 (this is just chunk_width * chunk_height)
	
	The source image is automatically divided into the required number of chunks based on these dimensions.
	
	For example, a 1920 × 1080 image with 640 × 480 chunks produces:
	
	chunks_x = 3
	chunks_y = 3
	
	Total chunks = 9
	
	The final row and column may contain partially filled chunks. These regions are zero-padded before being passed to ICER.

3. **Perform Compression**
	Call `Perform_ICER_Compress_From_SD()` somewhere to initiate the configured compression
	If you do not need performance benchmarking simply do not call the associated benchmarking methods
	
	
# Decompression

An associated non-embedded program to decompress ICER based on chunks is included. Simply run this on a ground station
and the program will automatically splice together decompressed chunks into the final image.
	
	