/*
 * img_jpeg.h
 *
 * Hardware JPEG encoder: RGB565 -> YCbCr 4:2:0 JPEG, streamed through the
 * STM32H7 codec 16 rows at a time and written straight to an open FatFs file.
 * Neither the raw frame nor the JPEG ever has to be resident in one piece.
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_IMG_JPEG_H_
#define CUSTOM_INC_IMG_JPEG_H_

#include "main.h"
#include "ff.h"

#define IMG_MAX_WIDTH		2560	// widest frame the band buffers are sized for

typedef enum {
	IMG_OK = 0,
	IMG_ERR_PARAM,		// bad size/quality, or img_jpeg_init() not called
	IMG_ERR_BUSY,		// an encode is already running (not reentrant)
	IMG_ERR_CODEC,		// HAL refused the configuration or the encode
	IMG_ERR_TIMEOUT,	// codec did not finish within IMG_JPEG_TIMEOUT_MS
	IMG_ERR_SOURCE,		// pixel source failed; output is incomplete
	IMG_ERR_WRITE,		// f_write failed; output file is incomplete
} img_status_t;

typedef struct {
	uint16_t width;		// pixels, multiple of 16, <= IMG_MAX_WIDTH
	uint16_t height;	// pixels, multiple of 16
	uint8_t  quality;	// 1-100
} img_jpeg_cfg_t;

typedef struct {
	uint32_t jpeg_bytes;	// size of the JPEG produced
	uint32_t encode_ms;		// wall time for the whole encode, incl. SD writes
	uint32_t encode_cycles;	// same, in CPU cycles (wraps after ~15.3 s at 280 MHz)
	uint32_t write_cycles;	// portion of encode_cycles spent inside f_write
} img_jpeg_result_t;

// Where pixels come from. The encoder pulls 16 rows at a time, top to bottom.
typedef struct img_source img_source_t;
struct img_source {
	// Copy the next `rows` rows of RGB565 into dst. Return 0 on success.
	int (*read_rows)(img_source_t *src, uint8_t *dst, uint32_t row_bytes, uint32_t rows);
	// Pixels are stored high byte first (e.g. OV7670 over DCMI) and must be
	// swapped to the little-endian RGB565 the converter expects.
	uint8_t swap_bytes;
	union {
		struct { const uint8_t *next; } sram;
		struct { FIL *fp; } file;
	} u;
};

/**
 * @brief  One-time setup: builds the colour-conversion tables and enables the
 *         cycle counter. Call once before any encode.
 */
img_status_t img_jpeg_init(JPEG_HandleTypeDef *hjpeg);

/**
 * @brief  Encode one frame from `src` into the file `dst` (opened for write).
 *         Blocks the calling task until done; other tasks keep running.
 *         Not reentrant: call from one task only.
 *         On any error the file is left partially written; the caller should
 *         close and delete it.
 */
img_status_t img_jpeg_encode(const img_jpeg_cfg_t *cfg, img_source_t *src,
		FIL *dst, img_jpeg_result_t *res);

/**
 * @brief  Source reading a whole frame already in RAM (e.g. a DCMI buffer).
 *         The frame must hold at least width*height*2 bytes.
 */
void img_source_sram(img_source_t *src, const void *frame, uint8_t swap_bytes);

/**
 * @brief  Source streaming a raw RGB565 file, so frames larger than RAM can be
 *         encoded. Rewinds the file to the start. Returns 0 on success.
 */
int img_source_file(img_source_t *src, FIL *fp, uint8_t swap_bytes);

#endif /* CUSTOM_INC_IMG_JPEG_H_ */
