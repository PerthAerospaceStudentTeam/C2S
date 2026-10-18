/*
 * img_jpeg.c
 *
 * Hardware JPEG encoder, ported from C2Sv0.1 jpeg_codec.c. Changes from v0.1:
 *  - colour tables and codec configuration are set up once, not per encode
 *  - pixels come from a pluggable source (RAM frame or raw file on SD)
 *  - output streams to the file through one small buffer instead of a 2 MB
 *    RAM buffer, which the H7 does not have
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#include "img_jpeg.h"
#include "jpeg_utils.h"
#include <string.h>

#define BAND_ROWS			16u			// one 4:2:0 MCU row
#define MCU_BLOCK_BYTES		384u		// 4:2:0 MCU = 6 blocks of 64 bytes
#define OUT_CHUNK_BYTES		8192u		// codec output written to SD in this size
#define IMG_JPEG_TIMEOUT_MS	60000u		// backstop; a 2560x1440 encode should be far below

// Buffers live in AXI SRAM (.bss), which the SDMMC IDMA can reach. 32-byte
// alignment keeps them cache-line aligned if D-Cache is turned on later.
ALIGN_32BYTES(static uint8_t s_band[IMG_MAX_WIDTH * BAND_ROWS * 2]);				// 80 KB raw RGB565
ALIGN_32BYTES(static uint8_t s_mcu[(IMG_MAX_WIDTH / 16) * MCU_BLOCK_BYTES]);		// 60 KB YCbCr
ALIGN_32BYTES(static uint8_t s_out[OUT_CHUNK_BYTES]);								// 8 KB JPEG

static JPEG_HandleTypeDef *s_hjpeg;
static JPEG_RGBToYCbCr_Convert_Function s_convert;

// Configuration currently loaded into the codec and jpeg_utils.
static struct {
	img_jpeg_cfg_t cfg;
	uint8_t valid;
} s_active;

// State for the encode in progress, shared with the HAL callbacks.
static struct {
	uint8_t      busy;
	img_source_t *src;
	FIL          *dst;
	uint32_t     row_bytes;
	uint32_t     rows_left;
	uint32_t     jpeg_bytes;
	uint32_t     write_cycles;
	img_status_t err;		// first error seen inside a callback
} s_enc;

/**
 * @brief  Swap the two bytes of every RGB565 pixel in place.
 *         `bytes` is a multiple of 4 and `buf` is word aligned.
 */
static void swap_rgb565(uint8_t *buf, uint32_t bytes)
{
	uint32_t *word = (uint32_t *)buf;

	for (uint32_t i = 0; i < bytes / 4u; i++) {
		word[i] = __REV16(word[i]);
	}
}

/**
 * @brief  Load the codec and colour converter for `cfg`, unless that is what
 *         they already hold. Both keep their state between encodes.
 */
static img_status_t configure(const img_jpeg_cfg_t *cfg)
{
	if (s_active.valid &&
		s_active.cfg.width == cfg->width &&
		s_active.cfg.height == cfg->height &&
		s_active.cfg.quality == cfg->quality) {
		return IMG_OK;
	}

	JPEG_ConfTypeDef conf = {0};
	conf.ColorSpace        = JPEG_YCBCR_COLORSPACE;
	conf.ChromaSubsampling = JPEG_420_SUBSAMPLING;
	conf.ImageWidth        = cfg->width;
	conf.ImageHeight       = cfg->height;
	conf.ImageQuality      = cfg->quality;

	s_active.valid = 0;

	if (HAL_JPEG_ConfigEncoding(s_hjpeg, &conf) != HAL_OK) {
		return IMG_ERR_CODEC;
	}

	uint32_t mcu_total;
	if (JPEG_GetEncodeColorConvertFunc(&conf, &s_convert, &mcu_total) != HAL_OK) {
		return IMG_ERR_CODEC;
	}

	s_active.cfg = *cfg;
	s_active.valid = 1;
	return IMG_OK;
}

/**
 * @brief  Fetch the next 16 rows and convert them to one row of YCbCr MCUs in
 *         s_mcu. Returns the number of bytes to hand the codec, 0 when the
 *         frame is finished.
 *
 *         The codec must receive every MCU it was configured for, otherwise
 *         the HAL pauses input and HAL_JPEG_Encode never returns. So if the
 *         source fails, the remaining rows are encoded as black and the error
 *         is reported afterwards.
 */
static uint32_t next_band(void)
{
	if (s_enc.rows_left == 0u) {
		return 0;
	}

	uint32_t band_bytes = s_enc.row_bytes * BAND_ROWS;

	if (s_enc.err == IMG_OK &&
		s_enc.src->read_rows(s_enc.src, s_band, s_enc.row_bytes, BAND_ROWS) != 0) {
		s_enc.err = IMG_ERR_SOURCE;
	}

	if (s_enc.err == IMG_ERR_SOURCE) {
		memset(s_band, 0, band_bytes);
	} else if (s_enc.src->swap_bytes) {
		swap_rgb565(s_band, band_bytes);
	}

	// BlockIndex 0 makes the converter treat s_band as MCU-row 0, i.e. rows
	// 0..15 of its own buffer. DataCount is the input size in bytes.
	uint32_t converted = 0;
	s_convert(s_band, s_mcu, 0, band_bytes, &converted);

	s_enc.rows_left -= BAND_ROWS;
	return converted;
}

/**
 * @brief  HAL calls this (in this task, polling mode) when the codec has
 *         consumed the previous band and needs more input.
 */
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData)
{
	(void)NbDecodedData;

	uint32_t n = next_band();
	HAL_JPEG_ConfigInputBuffer(hjpeg, s_mcu, n);	// n == 0: input finished
}

/**
 * @brief  HAL calls this when s_out is full, and once more at the end with
 *         whatever is left. Writes the chunk to the file and hands s_out back.
 *         f_write completes before returning, so one buffer is enough.
 */
void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength)
{
	if (s_enc.err == IMG_OK) {
		UINT written = 0;
		uint32_t start = DWT->CYCCNT;

		FRESULT fr = f_write(s_enc.dst, pDataOut, OutDataLength, &written);

		s_enc.write_cycles += DWT->CYCCNT - start;
		if (fr != FR_OK || written != OutDataLength) {
			s_enc.err = IMG_ERR_WRITE;
		}
	}

	s_enc.jpeg_bytes += OutDataLength;
	HAL_JPEG_ConfigOutputBuffer(hjpeg, s_out, sizeof(s_out));
}

img_status_t img_jpeg_init(JPEG_HandleTypeDef *hjpeg)
{
	if (hjpeg == NULL) {
		return IMG_ERR_PARAM;
	}

	s_hjpeg = hjpeg;
	s_active.valid = 0;

	JPEG_InitColorTables();

	// Cycle counter for timing. The M7 needs the DWT unlocked first.
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->LAR = 0xC5ACCE55;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	return IMG_OK;
}

img_status_t img_jpeg_encode(const img_jpeg_cfg_t *cfg, img_source_t *src,
		FIL *dst, img_jpeg_result_t *res)
{
	if (res != NULL) {
		memset(res, 0, sizeof(*res));
	}

	if (s_hjpeg == NULL || cfg == NULL || src == NULL || dst == NULL || res == NULL ||
		cfg->width == 0u || cfg->height == 0u ||
		(cfg->width % 16u) != 0u || (cfg->height % 16u) != 0u ||
		cfg->width > IMG_MAX_WIDTH ||
		cfg->quality == 0u || cfg->quality > 100u) {
		return IMG_ERR_PARAM;
	}

	if (s_enc.busy) {
		return IMG_ERR_BUSY;
	}

	img_status_t st = configure(cfg);
	if (st != IMG_OK) {
		return st;
	}

	s_enc.busy         = 1;
	s_enc.src          = src;
	s_enc.dst          = dst;
	s_enc.row_bytes    = (uint32_t)cfg->width * 2u;
	s_enc.rows_left    = cfg->height;
	s_enc.jpeg_bytes   = 0;
	s_enc.write_cycles = 0;
	s_enc.err          = IMG_OK;

	uint32_t start_ms = HAL_GetTick();
	uint32_t start_cycles = DWT->CYCCNT;

	// Prime the codec with band 0; GetDataCallback supplies the rest.
	uint32_t first = next_band();
	HAL_StatusTypeDef hal = HAL_JPEG_Encode(s_hjpeg, s_mcu, first,
			s_out, sizeof(s_out), IMG_JPEG_TIMEOUT_MS);

	res->encode_cycles = DWT->CYCCNT - start_cycles;
	res->encode_ms     = HAL_GetTick() - start_ms;
	res->jpeg_bytes    = s_enc.jpeg_bytes;
	res->write_cycles  = s_enc.write_cycles;

	s_enc.busy = 0;

	if (hal != HAL_OK) {
		s_active.valid = 0;		// reload the codec before the next encode
		return (hal == HAL_TIMEOUT) ? IMG_ERR_TIMEOUT : IMG_ERR_CODEC;
	}

	return s_enc.err;
}

static int sram_read_rows(img_source_t *src, uint8_t *dst, uint32_t row_bytes, uint32_t rows)
{
	uint32_t n = row_bytes * rows;

	memcpy(dst, src->u.sram.next, n);
	src->u.sram.next += n;
	return 0;
}

static int file_read_rows(img_source_t *src, uint8_t *dst, uint32_t row_bytes, uint32_t rows)
{
	UINT n = row_bytes * rows;
	UINT got = 0;

	if (f_read(src->u.file.fp, dst, n, &got) != FR_OK || got != n) {
		return -1;
	}
	return 0;
}

void img_source_sram(img_source_t *src, const void *frame, uint8_t swap_bytes)
{
	src->read_rows = sram_read_rows;
	src->swap_bytes = swap_bytes;
	src->u.sram.next = (const uint8_t *)frame;
}

int img_source_file(img_source_t *src, FIL *fp, uint8_t swap_bytes)
{
	if (f_lseek(fp, 0) != FR_OK) {
		return -1;
	}

	src->read_rows = file_read_rows;
	src->swap_bytes = swap_bytes;
	src->u.file.fp = fp;
	return 0;
}
