/*
 * img_capture.c
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#include "img_capture.h"
#include "cmsis_os2.h"

#define CAM_RGB565			0
#define CAM_QVGA			1
#define CAM_SETTLE_MS		300u
#define CAM_FRAME_TIMEOUT	1000u

#define CAM_AEC_VALUE		512u
#define CAM_GAIN_VALUE		0xA0u

extern DCMI_HandleTypeDef hdcmi;

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
	OV7670_Config(CAM_RGB565, CAM_QVGA, test_pattern, CAM_AEC_VALUE, CAM_GAIN_VALUE);
	osDelay(CAM_SETTLE_MS);

	int ret = -1;
	if (HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)frame, IMG_CAPTURE_BYTES / 4u) == HAL_OK &&
		wait_dcmi_ready(CAM_FRAME_TIMEOUT) == 0) {
		ret = 0;
	}

	HAL_DCMI_Stop(&hdcmi);
	HAL_GPIO_WritePin(DCMI_PWRDWN_GPIO_Port, DCMI_PWRDWN_Pin, GPIO_PIN_SET);
	return ret;
}
