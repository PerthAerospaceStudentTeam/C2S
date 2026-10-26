/*
 * imaging.c
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#include "imaging.h"
#include "img_capture.h"
#include "img_store.h"
#include "cmsis_os2.h"

volatile imaging_stats_t g_imaging_stats;

static img_store_slot_t s_slot;

static void capture_one(uint32_t *frame)
{
	const img_jpeg_cfg_t cfg = { IMG_CAPTURE_WIDTH, IMG_CAPTURE_HEIGHT, IMG_FLIGHT_QUALITY };
	img_source_t src = { .ram = (const uint8_t *)frame, .swap_bytes = IMG_CAPTURE_SWAP };
	img_jpeg_result_t res;

	if (img_capture_frame(frame, 0) != 0) {
		g_imaging_stats.capture_failed++;
		return;
	}

	if (img_store_begin(&s_slot) != STORE_OK) {
		g_imaging_stats.store_failed++;
		return;
	}

	img_status_t st = img_jpeg_encode(&cfg, &src, &s_slot.file, &res);
	if (st != IMG_OK) {
		img_store_abort(&s_slot);
		g_imaging_stats.encode_failed++;
		g_imaging_stats.last_error = st;
		return;
	}

	if (img_store_commit(&s_slot) != STORE_OK) {
		g_imaging_stats.store_failed++;
		return;
	}

	g_imaging_stats.captured++;
	g_imaging_stats.last_id = s_slot.id;
	g_imaging_stats.last_result = res;
}

void imaging_run(uint32_t *frame)
{
	for (;;) {
		capture_one(frame);
		osDelay(IMG_CAPTURE_PERIOD_MS);
	}
}
