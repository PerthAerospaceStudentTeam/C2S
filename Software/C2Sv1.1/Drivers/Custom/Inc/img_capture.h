/*
 * img_capture.h
 *
 * The one call the rest of the imaging code makes to the image sensor.
 * Swapping sensors means reimplementing img_capture_frame() and the defines
 * below; nothing else depends on the OV7670.
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_IMG_CAPTURE_H_
#define CUSTOM_INC_IMG_CAPTURE_H_

#include "main.h"

// Frame produced by img_capture_frame(): OV7670 at QVGA, RGB565.
#define IMG_CAPTURE_WIDTH	320u
#define IMG_CAPTURE_HEIGHT	240u
#define IMG_CAPTURE_BYTES	(IMG_CAPTURE_WIDTH * IMG_CAPTURE_HEIGHT * 2u)
#define IMG_CAPTURE_SWAP	1	// OV7670 delivers each pixel high byte first

/**
 * @brief  Capture one frame into `frame` (IMG_CAPTURE_BYTES, word aligned).
 *         Blocks the calling task, yielding while the DMA runs.
 *
 * @param  test_pattern (0) live image; (1) sensor colour-bar test pattern
 * @retval 0 on success, -1 if the sensor did not deliver a frame
 */
int img_capture_frame(uint32_t *frame, uint8_t test_pattern);

#endif /* CUSTOM_INC_IMG_CAPTURE_H_ */
