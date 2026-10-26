/*
 * link.c
 *
 *  Created on: 22 Oct 2026
 *      Author: gabe
 */

#include "link.h"
#include "icd.h"
#include "cmsis_os2.h"
#include <string.h>

extern SPI_HandleTypeDef hspi3;

static uint8_t  s_tx[ICD_FRAME_MAX];
static uint32_t s_tx_len;
static uint8_t  s_rx[ICD_FRAME_MAX];

static int cs_high(void)
{
	return HAL_GPIO_ReadPin(SPI3_CS_GPIO_Port, SPI3_CS_Pin) == GPIO_PIN_SET;
}

void link_init(void)
{
	memset(s_tx, 0, sizeof(s_tx));
	s_tx_len = 0;
}

uint32_t link_receive(uint8_t *buf, uint32_t cap)
{
	for (;;) {
		while (!cs_high()) {
			osDelay(1);
		}

		if (HAL_SPI_TransmitReceive_IT(&hspi3, s_tx, s_rx, ICD_FRAME_MAX) != HAL_OK) {
			HAL_SPI_Abort(&hspi3);
			osDelay(1);
			continue;
		}

		while (!(cs_high() && hspi3.RxXferCount < ICD_FRAME_MAX)) {
			osDelay(1);
		}

		uint32_t clocked = ICD_FRAME_MAX - hspi3.RxXferCount;
		HAL_SPI_Abort(&hspi3);

		if (s_tx_len > 0u && clocked >= s_tx_len) {
			memset(s_tx, 0, sizeof(s_tx));
			s_tx_len = 0;
		}

		uint32_t n = (clocked < cap) ? clocked : cap;
		memcpy(buf, s_rx, n);
		return n;
	}
}

void link_send(const uint8_t *frame, uint32_t len)
{
	memset(s_tx, 0, sizeof(s_tx));
	memcpy(s_tx, frame, len);
	s_tx_len = len;
}
