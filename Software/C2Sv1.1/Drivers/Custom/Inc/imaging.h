/*
 * imaging.h
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_IMAGING_H_
#define CUSTOM_INC_IMAGING_H_

#include "img_jpeg.h"

#define IMG_CAPTURE_PERIOD_MS	10000u
#define IMG_FLIGHT_QUALITY		25

typedef struct {
	uint32_t captured;
	uint32_t capture_failed;
	uint32_t store_failed;
	uint32_t encode_failed;
	img_status_t      last_error;
	uint32_t          last_id;
	img_jpeg_result_t last_result;
} imaging_stats_t;

extern volatile imaging_stats_t g_imaging_stats;

void imaging_run(uint32_t *frame);

#endif
