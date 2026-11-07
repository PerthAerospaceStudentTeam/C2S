/*
 * img_jpeg.h
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_IMG_JPEG_H_
#define CUSTOM_INC_IMG_JPEG_H_

#include "main.h"
#include "ff.h"

#define IMG_MAX_WIDTH		2560

/** Encoder return codes. */
typedef enum {
	IMG_OK = 0,
	IMG_ERR_PARAM,		/**< Invalid width or height */
	IMG_ERR_CODEC,		/**< JPEG peripheral error */
	IMG_ERR_SOURCE,		/**< Failed to read the source image */
	IMG_ERR_WRITE,		/**< Failed to write the output file */
} img_status_t;

/** Encoder settings. Width and height must be multiples of 16. */
typedef struct {
	uint16_t width;
	uint16_t height;
	uint8_t  quality;
} img_jpeg_cfg_t;

/** Encoder output statistics. */
typedef struct {
	uint32_t jpeg_bytes;
	uint32_t encode_ms;
} img_jpeg_result_t;

/** Image source: set either ram or file. */
typedef struct {
	const uint8_t *ram;
	FIL           *file;
	uint8_t        swap_bytes;	/**< 1 to swap the bytes of each RGB565 pixel */
} img_source_t;

void img_jpeg_init(JPEG_HandleTypeDef *hjpeg);

img_status_t img_jpeg_encode(const img_jpeg_cfg_t *cfg, img_source_t *src,
		FIL *dst, img_jpeg_result_t *res);

#endif
