/*
 * compress_test_harness.c
 *
 *  Created on: 20 July 2026
 *      Author: Dimitrije Simic
 */

#include "compress_test_harness.h"

static icer_output_data_buf_typedef output;

/**
 * Configure SOURCE_IMG_WIDTH, SOURCE_IMG_HEIGHT and filenames before compression
 */
#define SOURCE_IMG_WIDTH 1920
#define SOURCE_IMG_HEIGHT 1080
static char* filenames[] = {"Y_1920x1080.bin", "U_1920x1080.bin", "V_1920x1080.bin"};
#define CHUNK_WIDTH 640U
#define CHUNK_HEIGHT 480U


static uint16_t Get_Chunks_X(void)
{
	return (SOURCE_IMG_WIDTH + CHUNK_WIDTH - 1) / CHUNK_WIDTH;
}

static uint16_t Get_Chunks_Y(void)
{
	return (SOURCE_IMG_HEIGHT + CHUNK_HEIGHT - 1) / CHUNK_HEIGHT;
}


//static uint16_t Get_Dimension_Divisor(void) {
//	uint16_t divisor = 1;
//
//	while (((SOURCE_IMG_WIDTH / divisor) * (SOURCE_IMG_HEIGHT / divisor)) > MAX_CHUNK_PIXELS)
//	{
//		divisor++;
//	}
//
//	return divisor;
//}

/**
 * @brief Loads a rectangular image chunk from a file on the SD card.
 *
 * The image data is assumed to be stored in row-major order as uint16_t
 * pixels. The specified chunk is read row-by-row into the destination buffer.
 *
 * @param file          Pointer to the open SD card file.
 * @param chunk_row     Row index of the chunk within the image.
 * @param chunk_col     Column index of the chunk within the image.
 * @param chunk_width   Width of the chunk in pixels.
 * @param chunk_height  Height of the chunk in pixels.
 * @param buffer        Destination buffer for the loaded chunk.
 *
 * @return FX_SUCCESS on success, or a FileX error/status code on failure.
 */
static uint32_t Load_Data_Chunk_From_SD(FX_FILE* file, int chunk_row, int chunk_col, uint32_t chunk_width,
		uint32_t chunk_height, uint16_t* buffer)
{
	uint32_t status = 0;
	ULONG actual;

	// Calculate starting row and column of the chunk within the full image
	uint32_t image_row_start = chunk_row * chunk_height;
	uint32_t image_col_start = chunk_col * chunk_width;

	// Set the buffer to 0
	memset(buffer, 0, MAX_CHUNK_PIXELS * sizeof(uint16_t));


	// Clip valid_width or valid_height to maximum allowed chunk width/height
	// If valid is smaller than max allowed, then only read valid pixels
	uint32_t valid_width = SOURCE_IMG_WIDTH - image_col_start;
	if (valid_width > CHUNK_WIDTH)
	{
		valid_width = CHUNK_WIDTH;
	}

	uint32_t valid_height = SOURCE_IMG_HEIGHT - image_row_start;
	if (valid_height > CHUNK_HEIGHT)
	{
		valid_height = CHUNK_HEIGHT;
	}

	// Calculate number of bytes for one row of pixels in the chunk
	uint32_t row_size = valid_width * sizeof(uint16_t);

	// Read chunk one valid row at a time
	for (uint32_t row = 0; row < valid_height; row++)
	{
		// Calculate current rows position within full image
		uint32_t image_row = image_row_start + row;

		// Calculate byte offset of the first pixel of this row within the image
		// image_row * SOURCE_IMG_WIDTH = number of pixels before image_row
		// + image_col_start = move x pixels into image_row
		uint32_t offset = (image_row * SOURCE_IMG_WIDTH + image_col_start) * sizeof(uint16_t);

		// Move file pointer to beginning of row we want to read
		status = fx_file_seek(file, offset);

		if (status != FX_SUCCESS) {
			return status;
		}

		// read one row of the chunk directly into the buffer
		status = fx_file_read(file, &buffer[row * CHUNK_WIDTH], row_size, &actual);

		if (status != FX_SUCCESS)
		{
			return status;
		}

		if (actual != row_size)
		{
			return FX_END_OF_FILE;
		}
	}
	return FX_SUCCESS;
}

/**
 * @brief Compresses a YUV image from SD card storage in independent chunks.
 *
 * The Y, U and V image planes are stored as separate files on the SD card.
 * For each chunk position, the corresponding Y, U and V regions are loaded
 * into memory and compressed independently using ICER. The compressed output
 * for each chunk can then be written back to the SD card.
 *
 * @param y_file Pointer to the open Y-plane image file.
 * @param u_file Pointer to the open U-plane image file.
 * @param v_file Pointer to the open V-plane image file.
 *
 * @return FX_SUCCESS on successful completion, a FileX status code if
 *         loading a chunk fails, or an ICER result code if initialisation
 *         or compression fails.
 */
#define MAX_CHUNKS 20
static uint64_t chunk_compression_cycles[MAX_CHUNKS];
static uint32_t ICER_Compress_From_SD(FX_FILE* y_file, FX_FILE* u_file, FX_FILE* v_file, uint16_t chunks_x, uint16_t chunks_y) {
	uint32_t status = 0;
	int icer_res = 0;
	// fx_file for the compressed output binary
	FX_FILE output_file;
	const char* compressed_filename = "compressed.bin";

	// Clears old file if exists
	status = Init_Output_File(&output_file, compressed_filename);
	if (status != FX_SUCCESS) {
	    return status;
	}


	uint16_t chunk_width = CHUNK_WIDTH;
	uint16_t chunk_height = CHUNK_HEIGHT;

	uint8_t is_last_chunk = 0;

	// Holds the compressed output from ICER
	static uint8_t compressed_output[COMPRESSED_BUFFER_SIZE];
	static uint16_t Y[MAX_CHUNK_PIXELS];
	static uint16_t U[MAX_CHUNK_PIXELS];
	static uint16_t V[MAX_CHUNK_PIXELS];

	file_header_t file_header = {
			.chunks_x = chunks_x,
			.chunks_y = chunks_y,
			.chunk_width = chunk_width,
			.chunk_height = chunk_height,
			.original_width = SOURCE_IMG_WIDTH,
			.original_height = SOURCE_IMG_HEIGHT
	};

	// Store header data in output binary file
	status = SD_Stream_Data(&output_file, (char*)compressed_filename, &file_header, sizeof(file_header), 0);
	if (status != FX_SUCCESS) {
	    return status;
	}
	// Configurable ICER compression flags, requires testing for optimal output
	const int stages = 4;
	const enum icer_filter_types filt = ICER_FILTER_A;
	int segments = 10;


	// Holds data for each seperate chunk, gets written to output file
	chunk_header_t chunk_header;

	// Loop through each chunk, load data from SD card, compressed then write back to SD
	for (int row = 0; row < chunks_y; row++) {
		for (int col = 0; col < chunks_x; col++) {

			if (row == chunks_y - 1 && col == chunks_x - 1) {
				is_last_chunk = 1;
			}
			// reset each buffer
			memset(Y, 0, sizeof(Y));
			memset(U, 0, sizeof(U));
			memset(V, 0, sizeof(V));
			// Must call this for ICER, resets output structure each iteration
			memset(compressed_output, 0, sizeof(compressed_output));
			icer_res = icer_init_output_struct(&output, compressed_output, sizeof(compressed_output), ICER_BYTE_QUOTA);
			if (icer_res != ICER_RESULT_OK) return icer_res;

			status = Load_Data_Chunk_From_SD(y_file, row, col, chunk_width, chunk_height, Y);
			if (status != FX_SUCCESS) { return status; }

			status = Load_Data_Chunk_From_SD(u_file, row, col, chunk_width, chunk_height, U);
			if (status != FX_SUCCESS) { return status; }

			status = Load_Data_Chunk_From_SD(v_file, row, col, chunk_width, chunk_height, V);
			if (status != FX_SUCCESS) { return status; }

			// Perform actual compression
			uint32_t start = DWT->CYCCNT;
			icer_res = icer_compress_image_yuv_uint16(Y, U, V, CHUNK_WIDTH, CHUNK_HEIGHT, stages, filt, segments, &output);
			uint32_t end = DWT->CYCCNT;
			if (icer_res != ICER_RESULT_OK && icer_res != ICER_BYTE_QUOTA_EXCEEDED) return icer_res;

			uint16_t chunk_id = (row * chunks_x) + col;

			chunk_compression_cycles[chunk_id] = (uint64_t)(end - start);


			// Encode start of chunk with chunk header
			chunk_header.chunk_start = CHUNK_START_ID;
			chunk_header.chunk_id = chunk_id;
			chunk_header.compressed_size = output.size_used;
			// Save header to output file on SD
			status = SD_Stream_Data(&output_file, (char*)compressed_filename, &chunk_header, sizeof(chunk_header), 0);
			if (status != FX_SUCCESS) {
			    return status;
			}
			//Save compressed data to output file on SD
			status = SD_Stream_Data(&output_file, (char*)compressed_filename, output.rearrange_start, output.size_used, is_last_chunk);
			if (status != FX_SUCCESS) {
			    return status;
			}
		}
	}
	return status;
}

#define NUM_CLOCK_TESTS 1
static const uint32_t clock_dividers[NUM_CLOCK_TESTS] = {
		RCC_SYSCLK_DIV1,
		RCC_SYSCLK_DIV2,
		RCC_SYSCLK_DIV4,
		RCC_SYSCLK_DIV8
};
static uint64_t benchmark_clock[NUM_CLOCK_TESTS];
static uint64_t benchmark_cycles[NUM_CLOCK_TESTS];
static uint64_t benchmark_time_us[NUM_CLOCK_TESTS];

/*
 * @brief Writes benchmarking data to an SD card connected to board
 *
 * Assumes test was conducted with different clock speeds
 * and that the above buffers were populated with benchmarking data
 */
static uint32_t Write_Benchmark_To_SD() {
	uint32_t status;
	FX_FILE csv_file;
	const char* csv_filename = "benchmark.csv";
	char line[128];
	int len;

	// Clear output file
	status = Init_Output_File(&csv_file, csv_filename);
	if (status != FX_SUCCESS) {
		return status;
	}

	// Write headers to line buffer
	len = snprintf(line, sizeof(line), "prescalar,clock_hz,cycles,time_us\n");
	status = SD_Stream_Data(&csv_file, (char*)csv_filename, line, len, 0);
	if (status != FX_SUCCESS) {
		return status;
	}

	// For each clock test write data to csv file
	for (int i = 0; i < NUM_CLOCK_TESTS; i++) {
		uint8_t is_last = (i == NUM_CLOCK_TESTS - 1);

		// Write data to line buffer
		len = snprintf(line, sizeof(line), "%lu,%lu,%lu,%lu\r\n",
							(unsigned long)clock_dividers[i],
							(unsigned long)benchmark_clock[i],
							(unsigned long)benchmark_cycles[i],
							(unsigned long)benchmark_time_us[i]);

		status = SD_Stream_Data(&csv_file, (char*)csv_filename, line, len, is_last);
		if (status != FX_SUCCESS) {
			return status;
		}
	}
	return status;
}
#include <inttypes.h> // For PRIu64
static uint32_t Write_Chunks_To_SD(uint16_t num_chunks) {
	uint32_t status;
	FX_FILE chunk_csv_file;
	const char* csv_filename = "chunks_benchmark.csv";
	char line[128];
	int len;

	status = Init_Output_File(&chunk_csv_file, csv_filename);
	if (status != FX_SUCCESS) {
		return status;
	}

	len = snprintf(line, sizeof(line), "chunk#,cycles,time_us\r\n");
	status = SD_Stream_Data(&chunk_csv_file, (char*)csv_filename, line, len, 0);
	if (status != FX_SUCCESS) {
		return status;
	}

	for (int c = 0; c < num_chunks; c++) {
		uint8_t is_last = (c == num_chunks - 1);
        len = snprintf(line,sizeof(line),"%u,%lu,%lu\r\n",
            c,
            (unsigned long)chunk_compression_cycles[c],
            (unsigned long)(((uint64_t)chunk_compression_cycles[c] * 1000000ULL) / benchmark_clock[0]));

	    status = SD_Stream_Data(&chunk_csv_file, (char*)csv_filename, line, len, is_last);
		if (status != FX_SUCCESS) {
			return status;
		}
	}
	return status;
}

/*
 * @brief Entry point to the ICER compression benchmark
 *
 * For each clock speed to be tested, the target Y, U and V
 * image .bins are reset to the beginning and the entire image
 * is compressed chunk-by-chunk
 *
 * The total compression cycles from each chunk are summed
 * to obtain the total compression cycles for the image. Total
 * compression time is then calculated from the measured CPU
 * clock frequency
 *
 * Benchmark results for each clock speed are written to SD card
 * after all clock tests have completed
 *
 * Change the YUV filenames and SOURCE_IMG_* dimensions to
 * benchmark different image sizes
 *
 */
uint32_t Perform_ICER_Compress_From_SD(void) {

	uint32_t status = 0;

	FX_FILE  y_file;
	FX_FILE  u_file;
	FX_FILE  v_file;

	// Open uncompressed Y  U and V files
	// Represents the amount of chunks per row and column
	uint16_t chunks_x = Get_Chunks_X();
	uint16_t chunks_y = Get_Chunks_Y();
	uint16_t num_chunks = chunks_x * chunks_y;

	status = SD_Open_YUV_Files(filenames, &y_file, &u_file, &v_file);
	if (status != FX_SUCCESS) {
		return status;
	}
	// Start benchmark
    for (int i = 0; i < NUM_CLOCK_TESTS; i++) {

    	// For benchmarking different clock speeds
    	Set_Prescalar(clock_dividers[i]);
    	benchmark_clock[i] = SystemCoreClock;

        status = fx_file_seek(&y_file, 0);
        if (status != FX_SUCCESS) { return status; }
        status = fx_file_seek(&u_file, 0);
        if (status != FX_SUCCESS) { return status; }
        status = fx_file_seek(&v_file, 0);
        if (status != FX_SUCCESS) { return status; }

    	status = ICER_Compress_From_SD(&y_file, &u_file, &v_file, chunks_x, chunks_y);
        if (status != FX_SUCCESS) {
        	fx_file_close(&y_file);
        	fx_file_close(&u_file);
        	fx_file_close(&v_file);
            return status;
        }
        status = Write_Chunks_To_SD(num_chunks);
        if (status != FX_SUCCESS) { return status; }
        // Sum data from each chunk
        uint64_t total_cycles = 0;
        for (int c = 0; c < num_chunks; c++) {

        	total_cycles += chunk_compression_cycles[c];
        }

        // Store in benchmark data arrays
        benchmark_cycles[i] = total_cycles;
        benchmark_time_us[i] =
            ((uint64_t)total_cycles * 1000000ULL) / benchmark_clock[i];
    }

	// End benchmark
    Write_Benchmark_To_SD();
	fx_file_close(&y_file);
	fx_file_close(&u_file);
	fx_file_close(&v_file);
	return status;
}
