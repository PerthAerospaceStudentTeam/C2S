/*
 * imaging.h
 *
 * Flight imaging loop: on a fixed cadence, capture a frame, encode it to
 * JPEG and commit it to the image store for comms to serve later.
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_IMAGING_H_
#define CUSTOM_INC_IMAGING_H_

#include "img_jpeg.h"

#define IMG_CAPTURE_PERIOD_MS	10000u	// placeholder: capture cadence not yet decided
#define IMG_FLIGHT_QUALITY		25		// placeholder until Phase 7 picks the sweet spot

typedef struct {
	uint32_t captured;			// images committed to the store
	uint32_t skipped_power;		// cycles skipped by the voltage check
	uint32_t capture_failed;
	uint32_t store_failed;		// could not open or commit the file
	uint32_t encode_failed;
	img_status_t      last_error;
	uint32_t          last_id;
	img_jpeg_result_t last_result;
} imaging_stats_t;

extern volatile imaging_stats_t g_imaging_stats;

/**
 * @brief  Run the flight loop forever. `frame` is the capture buffer
 *         (IMG_CAPTURE_BYTES, word aligned). Needs img_jpeg_init() and
 *         img_store_init() done first.
 */
void imaging_run(uint32_t *frame);

#endif /* CUSTOM_INC_IMAGING_H_ */
