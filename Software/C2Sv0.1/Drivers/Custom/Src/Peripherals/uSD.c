/*
 * uSD.c
 *
 *  Created on: Mar 8, 2026
 *      Author: felix
 */

#include "uSD.h"
#include "compress_test_harness.h"


/* FileX handles and memory */
FX_MEDIA        sd_disk;
FX_FILE         my_file;
#define CHUNK_SIZE 4096
static uint8_t         media_memory[CHUNK_SIZE]; // was 4096

extern VOID fx_stm32_sd_driver(FX_MEDIA *media_ptr);

uint32_t uSD_Init(void) {
    fx_system_initialize();

    /* Initialises the SD driver and opens the media */
    return fx_media_open(&sd_disk, "STM32_SPI_SD", fx_stm32_sd_driver,
                         (VOID *)FX_NULL, media_memory, sizeof(media_memory));
}

uint32_t Init_Output_File(FX_FILE* output_file, const char* filename) {
	uint32_t status;

	status = fx_file_delete(&sd_disk, filename);

    if (status != FX_SUCCESS && status != FX_NOT_FOUND) {
        return status;
    }

    return FX_SUCCESS;

}

void uSD_Test(void) {
    uint32_t status;

    // 1. Initialise and Open Media
    //status = uSD_Init();
//    if (status != FX_SUCCESS) {
//        return;
//    }

    // 2. Create and Open File
    // fx_file_create will return an error if file exists, we ignore it and just open.
    fx_file_create(&sd_disk, "TEST.TXT");
    status = fx_file_open(&sd_disk, &my_file, "TEST.TXT", FX_OPEN_FOR_WRITE);

    if (status == FX_SUCCESS) {
        // 3. Write Data
        char *msg = "Hello from STM32U5 to SD Card using FileX!\r\n";
        fx_file_write(&my_file, msg, strlen(msg));

        // 4. Close File and Flush Cache to Physical Media
        fx_file_close(&my_file);
        fx_media_flush(&sd_disk);
    }

    // 5. Safely Unmount
    fx_media_close(&sd_disk);
}

/**
 * @brief Streams data to a binary file on the SD card.
 *
 * @param filename: Name of the file (e.g., "DATA.BIN")
 * @param data: Pointer to the 8-bit or 16-bit buffer
 * @param size_in_bytes: Total size to write (use: num_elements * sizeof(element))
 * @param is_last_chunk: If 1, closes the file; if 0, keeps it open for more data.
 */
uint32_t SD_Stream_Data(FX_FILE* file, char* filename, void* data, uint32_t size_in_bytes, uint8_t is_last_chunk) {
    uint32_t status;
    static uint8_t file_is_open = 0;

    // Initialise SD if necessary
    if (sd_disk.fx_media_id != FX_MEDIA_ID) {
        status = uSD_Init();
        if (status != FX_SUCCESS) return status;
    }

    // 2. Open or Create the file
    // FX_OPEN_FOR_WRITE starts at the beginning.
    // Use fx_file_relative_seek(&my_file, 0, FX_SEEK_END) if appending to existing data.
    if (!file_is_open) {
		status = fx_file_open(&sd_disk, file, filename, FX_OPEN_FOR_WRITE);
		if (status == FX_NOT_FOUND) {
			// File doesn't exist, create it
			status = fx_file_create(&sd_disk, filename);

			if (status != FX_SUCCESS) {
				return status;
			}
			// Now open newly created file
			status = fx_file_open(&sd_disk, file, filename, FX_OPEN_FOR_WRITE);
			if (status != FX_SUCCESS) {
				return status;
			}

		}
		else if (status != FX_SUCCESS) {
        	return status;
        }
        file_is_open = 1;
    }

    // Seek to end of file
	status = fx_file_relative_seek(file, 0, FX_SEEK_END);
	if (status != FX_SUCCESS) {
		file_is_open = 0;
		return status;
	}

    // 3. Write the buffer
    // FileX handles the byte-alignment internally
    status = fx_file_write(file, data, size_in_bytes);
    if (status != FX_SUCCESS) {
        fx_file_close(file);
        file_is_open = 0;
        return status;
    }
    // 4. Check for last chunk and handle
    if (is_last_chunk) {
        status = fx_file_close(file);
        if (status != FX_SUCCESS) {
            file_is_open = 0;
            return status;
        }

        status = fx_media_flush(&sd_disk);
        file_is_open = 0;
    }

    return status;
}

/**
 * @brief Reads a binary file from the SD card into a buffer.
 *
 * @param filename: Name of the file (e.g., "LEO.YUV")
 * @param data: Destination buffer
 * @param size_in_bytes: Expected size to read
 */
uint32_t SD_Read_Data(char* filename, void* data, uint32_t size_in_bytes) {
    uint32_t status;
    uint32_t bytes_read = 0;

    if (sd_disk.fx_media_id != FX_MEDIA_ID) {
        status = uSD_Init();
        if (status != FX_SUCCESS) return status;
    }

    status = fx_file_open(&sd_disk, &my_file, filename, FX_OPEN_FOR_READ);
    if (status != FX_SUCCESS) return status;

    status = fx_file_read(&my_file, data, size_in_bytes, &bytes_read);
    fx_file_close(&my_file);

    if (status == FX_SUCCESS && bytes_read != size_in_bytes) {
        return FX_END_OF_FILE;
    }

    return status;
}

uint32_t SD_Open_YUV_Files(char* filenames[], FX_FILE* y_file, FX_FILE* u_file, FX_FILE* v_file)
{
	uint32_t status = 0;

	status = fx_file_open(&sd_disk, y_file, filenames[0], FX_OPEN_FOR_READ);
	if (status != FX_SUCCESS) return status;

	status = fx_file_open(&sd_disk, u_file, filenames[1], FX_OPEN_FOR_READ);
	if (status != FX_SUCCESS) {
		fx_file_close(y_file);
		return status;
	}

	status = fx_file_open(&sd_disk, v_file, filenames[2], FX_OPEN_FOR_READ);
	if (status != FX_SUCCESS) {
        fx_file_close(y_file);
        fx_file_close(u_file);
		return status;
	}

	return FX_SUCCESS;
}


/**
 * @brief Simulates an image data stream and writes it to the SD card.
 */
void Test_Image_Stream(void) {
    uint32_t status;
    uint16_t dummy_image_row[256]; // 256 pixels * 2 bytes = 512 bytes (one sector)
    uint16_t num_rows = 100;       // Total "image" height

    // 1. Initialise the SD card
    status = uSD_Init();
    if (status != FX_SUCCESS) return;

    // 2. Create and Open a binary file
    //fx_file_create(&sd_disk, "IMAGE.BIN");
    status = fx_file_open(&sd_disk, &my_file, "IMAGE.BIN", FX_OPEN_FOR_WRITE);
    if (status != FX_SUCCESS) return;

    // 3. Stream the "Image" row by row
    for (uint16_t i = 0; i < num_rows; i++) {
        // Generate a synthetic pattern (e.g., a gradient)
        for (uint16_t j = 0; j < 256; j++) {
            dummy_image_row[j] = (uint16_t)(i + j);
        }

        // Write the 512-byte row to the SD card
        status = fx_file_write(&my_file, dummy_image_row, sizeof(dummy_image_row));

        if (status != FX_SUCCESS) break;
    }

    // 4. Finalize and Flush
    fx_file_close(&my_file);
    fx_media_flush(&sd_disk);
    fx_media_close(&sd_disk);
}
