/*
 * img_selftest.c
 *
 * Phase 1 bring-up test for the hardware JPEG path. See img_selftest.h.
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#include "img_selftest.h"
#include "img_capture.h"
#include "img_store.h"
#include <string.h>

volatile selftest_report_t g_img_selftest;

// FIL objects are ~550 bytes each; keep them off the task stack.
static FIL s_out_file;
static FIL s_in_file;

/**
 * @brief  Encode from `src` into a new file `name`. Deletes the file if the
 *         encode fails, so a bad JPEG is never left on the card.
 */
static void encode_to_file(const char *name, const img_jpeg_cfg_t *cfg,
		img_source_t *src, volatile selftest_case_t *tc)
{
	img_jpeg_result_t res;

	if (f_open(&s_out_file, name, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
		tc->outcome = SELFTEST_FAIL_OPEN;
		return;
	}

	img_status_t st = img_jpeg_encode(cfg, src, &s_out_file, &res);

	if (f_close(&s_out_file) != FR_OK && st == IMG_OK) {
		st = IMG_ERR_WRITE;
	}
	if (st != IMG_OK) {
		f_unlink(name);
	}

	tc->status = st;
	tc->result = res;
	tc->outcome = (st == IMG_OK) ? SELFTEST_PASS : SELFTEST_FAIL_ENCODE;
}

/**
 * @brief  Capture a frame and encode it.
 */
static void test_camera(const char *name, uint32_t *frame, uint8_t test_pattern,
		volatile selftest_case_t *tc)
{
	const img_jpeg_cfg_t cfg = { IMG_CAPTURE_WIDTH, IMG_CAPTURE_HEIGHT, IMG_SELFTEST_QUALITY };
	img_source_t src;

	if (img_capture_frame(frame, test_pattern) != 0) {
		tc->outcome = SELFTEST_FAIL_CAPTURE;
		return;
	}

	img_source_sram(&src, frame, IMG_CAPTURE_SWAP);
	encode_to_file(name, &cfg, &src, tc);
}

/**
 * @brief  Encode the v0.1 benchmark frame straight from SD. Those files were
 *         written little-endian (v0.1 encoded them without a swap).
 */
static void test_raw_file(volatile selftest_case_t *tc)
{
	const img_jpeg_cfg_t cfg = { 640, 480, IMG_SELFTEST_QUALITY };
	img_source_t src;

	if (f_open(&s_in_file, "640x480_640x480.raw", FA_READ) != FR_OK) {
		tc->outcome = SELFTEST_SKIPPED;
		return;
	}

	if (img_source_file(&src, &s_in_file, 0) != 0) {
		tc->outcome = SELFTEST_FAIL_OPEN;
	} else {
		encode_to_file("RAW640.JPG", &cfg, &src, tc);
	}

	f_close(&s_in_file);
}

void img_selftest_run(uint32_t *frame)
{
	memset((void *)&g_img_selftest, 0, sizeof(g_img_selftest));

	g_img_selftest.store_ready = img_store_ready();
	if (!g_img_selftest.store_ready) {
		return;
	}

	test_camera("BARS.JPG", frame, 1, &g_img_selftest.bars);
	test_camera("LIVE.JPG", frame, 0, &g_img_selftest.live);
	test_raw_file(&g_img_selftest.raw640);
}
