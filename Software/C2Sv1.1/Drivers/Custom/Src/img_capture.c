/*
 * img_capture.c
 *
 * OV7670 implementation of img_capture_frame(). Same sequence as the
 * dashboard loop in main.c, plus a settle delay and timeouts.
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#include "img_capture.h"
#include "cmsis_os2.h"

#define CAM_RGB565			0
#define CAM_QVGA			1
#define CAM_SETTLE_MS		300u	// OV7670_Config resets the sensor; let exposure settle
#define CAM_FRAME_TIMEOUT	1000u

extern DCMI_HandleTypeDef hdcmi;

/**
 * @brief  Wait for the DCMI to return to READY, yielding to other tasks.
 *         Returns 0 when ready, -1 on timeout.
 */
static int wait_dcmi_ready(uint32_t timeout_ms)
{
	uint32_t start = HAL_GetTick();

	while (hdcmi.State != HAL_DCMI_STATE_READY) {
		if (HAL_GetTick() - start > timeout_ms) {
			return -1;
		}
		osDelay(1);
	}
	return 0;
}

int img_capture_frame(uint32_t *frame, uint8_t test_pattern)
{
	HAL_GPIO_WritePin(DCMI_PWRDWN_GPIO_Port, DCMI_PWRDWN_Pin, GPIO_PIN_RESET);
	OV7670_Config(CAM_RGB565, CAM_QVGA, test_pattern);
	osDelay(CAM_SETTLE_MS);

	int ret = -1;
	if (wait_dcmi_ready(CAM_FRAME_TIMEOUT) == 0 &&
		HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)frame, IMG_CAPTURE_BYTES / 4u) == HAL_OK &&
		wait_dcmi_ready(CAM_FRAME_TIMEOUT) == 0) {
		ret = 0;
	}

	HAL_DCMI_Stop(&hdcmi);
	HAL_GPIO_WritePin(DCMI_PWRDWN_GPIO_Port, DCMI_PWRDWN_Pin, GPIO_PIN_SET);
	return ret;
}
