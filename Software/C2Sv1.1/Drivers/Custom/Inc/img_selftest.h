/*
 * img_selftest.h
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_IMG_SELFTEST_H_
#define CUSTOM_INC_IMG_SELFTEST_H_

#include "img_jpeg.h"

#define IMG_SELFTEST			1
#define IMG_SELFTEST_QUALITY	75

typedef struct {
	uint8_t           captured;
	img_status_t      status;
	img_jpeg_result_t result;
} selftest_case_t;

typedef struct {
	uint8_t         store_ready;
	selftest_case_t bars;
	selftest_case_t live;
} selftest_report_t;

extern volatile selftest_report_t g_img_selftest;

void img_selftest_run(uint32_t *frame);

#endif
