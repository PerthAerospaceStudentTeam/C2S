/*
 * img_selftest.c
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#include "img_selftest.h"
#include "img_capture.h"
#include "img_store.h"

volatile selftest_report_t g_img_selftest;

static FIL s_file;

static void run_case(const char *name, uint32_t *frame, uint8_t test_pattern,
		volatile selftest_case_t *tc)
{
	const img_jpeg_cfg_t cfg = { IMG_CAPTURE_WIDTH, IMG_CAPTURE_HEIGHT, IMG_SELFTEST_QUALITY };
	img_source_t src = { .ram = (const uint8_t *)frame, .swap_bytes = IMG_CAPTURE_SWAP };
	img_jpeg_result_t res = {0};

	tc->captured = (img_capture_frame(frame, test_pattern) == 0);
	if (!tc->captured) {
		return;
	}

	img_status_t st = IMG_ERR_WRITE;
	if (f_open(&s_file, name, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
		st = img_jpeg_encode(&cfg, &src, &s_file, &res);
		if (f_close(&s_file) != FR_OK && st == IMG_OK) {
			st = IMG_ERR_WRITE;
		}
	}

	tc->status = st;
	tc->result = res;
}

void img_selftest_run(uint32_t *frame)
{
	g_img_selftest.store_ready = img_store_ready();
	if (!g_img_selftest.store_ready) {
		return;
	}

	run_case("BARS.JPG", frame, 1, &g_img_selftest.bars);
	run_case("LIVE.JPG", frame, 0, &g_img_selftest.live);
}
