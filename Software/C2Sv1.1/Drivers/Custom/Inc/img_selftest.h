/*
 * img_selftest.h
 *
 * Phase 1 bring-up test for the hardware JPEG path. Runs once at the start of
 * the imaging task and writes three JPEGs to the SD card:
 *   BARS.JPG   - OV7670 colour-bar test pattern (known input: checks colours)
 *   LIVE.JPG   - a live camera frame
 *   RAW640.JPG - 640x480_640x480.raw from the v0.1 benchmark set, if present
 *                (checks the SD-file source; skipped if the file is missing)
 * Results land in g_img_selftest for inspection in the debugger.
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_IMG_SELFTEST_H_
#define CUSTOM_INC_IMG_SELFTEST_H_

#include "img_jpeg.h"

#define IMG_SELFTEST			1		// (0) Off; (1) Run once at imaging task start
#define IMG_SELFTEST_QUALITY	75

typedef enum {
	SELFTEST_NOT_RUN = 0,
	SELFTEST_PASS,
	SELFTEST_FAIL_CAPTURE,	// DCMI never finished the frame
	SELFTEST_FAIL_OPEN,		// could not create the output (or open the input) file
	SELFTEST_FAIL_ENCODE,	// see .status for the img_status_t
	SELFTEST_SKIPPED,		// optional input file not on the card
} selftest_outcome_t;

typedef struct {
	selftest_outcome_t outcome;
	img_status_t       status;
	img_jpeg_result_t  result;
} selftest_case_t;

typedef struct {
	uint8_t         store_ready;	// 0: SD not mounted, nothing else ran
	selftest_case_t bars;
	selftest_case_t live;
	selftest_case_t raw640;
} selftest_report_t;

extern volatile selftest_report_t g_img_selftest;

/**
 * @brief  Run the Phase 1 tests. `frame` is the capture buffer
 *         (IMG_CAPTURE_BYTES, word aligned). Needs the scheduler running,
 *         img_jpeg_init() and img_store_init() (which mounts the card) done.
 */
void img_selftest_run(uint32_t *frame);

#endif /* CUSTOM_INC_IMG_SELFTEST_H_ */
