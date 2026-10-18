/*
 * imaging.c
 *
 * Flight imaging loop. See imaging.h.
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#include "imaging.h"
#include "img_capture.h"
#include "img_store.h"
#include "cmsis_os2.h"

volatile imaging_stats_t g_imaging_stats;

// FIL is ~550 bytes; keep the slot off the task stack.
static img_store_slot_t s_slot;

/**
 * @brief  Whether there is enough power to capture.
 *         U10: threshold not decided. Phase 5 moves INA219 sampling into the
 *         housekeeping task; this will read its latest value.
 */
static int power_ok(void)
{
	return 1;
}

/**
 * @brief  One capture -> encode -> commit cycle.
 */
static void capture_one(uint32_t *frame)
{
	static const img_jpeg_cfg_t cfg = {
		IMG_CAPTURE_WIDTH, IMG_CAPTURE_HEIGHT, IMG_FLIGHT_QUALITY
	};

	if (!power_ok()) {
		g_imaging_stats.skipped_power++;
		return;
	}

	if (img_capture_frame(frame, 0) != 0) {
		g_imaging_stats.capture_failed++;
		return;
	}

	if (img_store_begin(&s_slot) != STORE_OK) {
		g_imaging_stats.store_failed++;
		return;
	}

	img_source_t src;
	img_jpeg_result_t res;
	img_source_sram(&src, frame, IMG_CAPTURE_SWAP);

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
	uint32_t next_wake = osKernelGetTickCount();

	for (;;) {
		capture_one(frame);

		// Fixed cadence measured from the start of each cycle. If a cycle
		// overran, start the next one now rather than trying to catch up.
		next_wake += IMG_CAPTURE_PERIOD_MS;
		uint32_t now = osKernelGetTickCount();
		if ((int32_t)(next_wake - now) <= 0) {
			next_wake = now;
		} else {
			osDelayUntil(next_wake);
		}
	}
}
