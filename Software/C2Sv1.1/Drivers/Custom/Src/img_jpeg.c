/*
 * img_jpeg.c
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#include "img_jpeg.h"
#include "jpeg_utils.h"
#include <string.h>

#define BAND_ROWS			16u
#define MCU_BLOCK_BYTES		384u
#define OUT_CHUNK_BYTES		8192u
#define ENCODE_TIMEOUT_MS	60000u

ALIGN_32BYTES(static uint8_t s_band[IMG_MAX_WIDTH * BAND_ROWS * 2]);
ALIGN_32BYTES(static uint8_t s_mcu[(IMG_MAX_WIDTH / 16) * MCU_BLOCK_BYTES]);
ALIGN_32BYTES(static uint8_t s_out[OUT_CHUNK_BYTES]);

static JPEG_HandleTypeDef *s_hjpeg;
static JPEG_RGBToYCbCr_Convert_Function s_convert;

static img_source_t *s_src;
static FIL          *s_dst;
static uint32_t      s_band_bytes;
static uint32_t      s_rows_left;
static uint32_t      s_jpeg_bytes;
static img_status_t  s_err;

static int read_band(void)
{
	if (s_src->ram != NULL) {
		memcpy(s_band, s_src->ram, s_band_bytes);
		s_src->ram += s_band_bytes;
		return 0;
	}

	UINT got = 0;
	if (f_read(s_src->file, s_band, s_band_bytes, &got) != FR_OK || got != s_band_bytes) {
		return -1;
	}
	return 0;
}

static uint32_t next_band(void)
{
	if (s_rows_left == 0u) {
		return 0;
	}

	if (s_err == IMG_OK && read_band() != 0) {
		s_err = IMG_ERR_SOURCE;
	}

	if (s_err == IMG_ERR_SOURCE) {
		memset(s_band, 0, s_band_bytes);
	} else if (s_src->swap_bytes) {
		uint32_t *word = (uint32_t *)s_band;
		for (uint32_t i = 0; i < s_band_bytes / 4u; i++) {
			word[i] = __REV16(word[i]);
		}
	}

	uint32_t converted = 0;
	s_convert(s_band, s_mcu, 0, s_band_bytes, &converted);

	s_rows_left -= BAND_ROWS;
	return converted;
}

void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData)
{
	(void)NbDecodedData;

	HAL_JPEG_ConfigInputBuffer(hjpeg, s_mcu, next_band());
}

void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength)
{
	if (s_err == IMG_OK) {
		UINT written = 0;
		if (f_write(s_dst, pDataOut, OutDataLength, &written) != FR_OK || written != OutDataLength) {
			s_err = IMG_ERR_WRITE;
		}
	}

	s_jpeg_bytes += OutDataLength;
	HAL_JPEG_ConfigOutputBuffer(hjpeg, s_out, sizeof(s_out));
}

void img_jpeg_init(JPEG_HandleTypeDef *hjpeg)
{
	s_hjpeg = hjpeg;
	JPEG_InitColorTables();
}

img_status_t img_jpeg_encode(const img_jpeg_cfg_t *cfg, img_source_t *src,
		FIL *dst, img_jpeg_result_t *res)
{
	res->jpeg_bytes = 0;
	res->encode_ms = 0;

	if ((cfg->width % 16u) != 0u || (cfg->height % 16u) != 0u || cfg->width > IMG_MAX_WIDTH) {
		return IMG_ERR_PARAM;
	}

	JPEG_ConfTypeDef conf = {0};
	conf.ColorSpace        = JPEG_YCBCR_COLORSPACE;
	conf.ChromaSubsampling = JPEG_420_SUBSAMPLING;
	conf.ImageWidth        = cfg->width;
	conf.ImageHeight       = cfg->height;
	conf.ImageQuality      = cfg->quality;

	uint32_t mcu_total;
	if (HAL_JPEG_ConfigEncoding(s_hjpeg, &conf) != HAL_OK ||
		JPEG_GetEncodeColorConvertFunc(&conf, &s_convert, &mcu_total) != HAL_OK) {
		return IMG_ERR_CODEC;
	}

	s_src        = src;
	s_dst        = dst;
	s_band_bytes = (uint32_t)cfg->width * 2u * BAND_ROWS;
	s_rows_left  = cfg->height;
	s_jpeg_bytes = 0;
	s_err        = IMG_OK;

	uint32_t start = HAL_GetTick();
	uint32_t first = next_band();
	HAL_StatusTypeDef hal = HAL_JPEG_Encode(s_hjpeg, s_mcu, first,
			s_out, sizeof(s_out), ENCODE_TIMEOUT_MS);

	res->encode_ms  = HAL_GetTick() - start;
	res->jpeg_bytes = s_jpeg_bytes;

	if (hal != HAL_OK) {
		return IMG_ERR_CODEC;
	}
	return s_err;
}
