/*
 * imaging.h
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 *
 */

#ifndef CUSTOM_INC_IMAGING_H_
#define CUSTOM_INC_IMAGING_H_

#include "img_jpeg.h"

#include "main.h"

// --- Capture ---
#define IMG_CAPTURE_WIDTH		320u
#define IMG_CAPTURE_HEIGHT		240u
#define IMG_CAPTURE_BYTES		(IMG_CAPTURE_WIDTH * IMG_CAPTURE_HEIGHT * 2u)
#define IMG_CAPTURE_SWAP		1
#define IMG_CAPTURE_PERIOD_MS	100u
#define IMG_FLIGHT_QUALITY		25

// --- Self test ---
#define IMG_SELFTEST			0
#define IMG_SELFTEST_QUALITY	75

// --- Image Queue ---
#define IMG_SRC_NIL		0
#define IMG_SRC_FISH 	1
#define IMG_SRC_C2S 	2

/** Running capture/store/encode counters. */
typedef struct {
	uint32_t captured;
	uint32_t capture_failed;
	uint32_t store_failed;
	uint32_t encode_failed;
	img_status_t      last_error;
	uint32_t          last_id;
	img_jpeg_result_t last_result;
} imaging_stats_t;

/** Outcome of one self-test image. */
typedef struct {
	uint8_t           captured;
	img_status_t      status;
	img_jpeg_result_t result;
} selftest_case_t;

/** Self-test report: colour bars test pattern and live image. */
typedef struct {
	uint8_t         store_ready;
	selftest_case_t bars;
	selftest_case_t live;
} selftest_report_t;

/** Image queue request. */
typedef struct {
	uint16_t	buf; 		// How many images are in the queue
	uint8_t 	src; 		// Where the image was requested from (FISH or Housekeeping Task)
	uint16_t 	idx; 		// File index of saved image
	uint32_t	timestamp; 	// When the image was taken (tick)
} imaging_req_t;

extern volatile imaging_stats_t g_imaging_stats;
extern volatile selftest_report_t g_img_selftest;

int img_capture_frame(uint32_t *frame, uint8_t test_pattern);

void img_selftest_run(uint32_t *frame);

void camera_capture_continuous(uint32_t *frame);

void camera_dashboard_operate();

#endif
