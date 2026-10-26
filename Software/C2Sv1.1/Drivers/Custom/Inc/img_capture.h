/*
 * img_capture.h
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_IMG_CAPTURE_H_
#define CUSTOM_INC_IMG_CAPTURE_H_

#include "main.h"

#define IMG_CAPTURE_WIDTH	320u
#define IMG_CAPTURE_HEIGHT	240u
#define IMG_CAPTURE_BYTES	(IMG_CAPTURE_WIDTH * IMG_CAPTURE_HEIGHT * 2u)
#define IMG_CAPTURE_SWAP	1

int img_capture_frame(uint32_t *frame, uint8_t test_pattern);

#endif
