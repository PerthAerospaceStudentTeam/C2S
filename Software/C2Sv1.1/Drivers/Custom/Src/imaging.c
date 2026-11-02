/*
 * imaging.c
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 *
 *  Modified on: 28/10/2026
 *  	Author: felix
 */

#include "imaging.h"

#include "img_capture.h"
#include "img_store.h"
#include "cmsis_os2.h"
#include "ina219.h"
#include "usbd_cdc_if.h"
#include "dashboard.h"
#include "ov7670.h"

volatile imaging_stats_t g_imaging_stats;	// Running capture/store/encode counters
static img_store_slot_t s_slot;				// Current SD card slot being written

// --- USB CDC ---
extern uint8_t recvDone;					// Set by the USB RX callback when a packet arrives
extern uint8_t recvLen;
extern uint8_t UserRxBufferHS[APP_RX_DATA_SIZE];
// --- INA219 ---
extern CSA_HandleTypeDef csa;
// --- DCMI ---
extern DCMI_HandleTypeDef hdcmi;
extern uint32_t pBuffer[VGA_IMAGE_SIZE_WORDS];
uint32_t camera_config = 0;
uint32_t camera_config_prev = 0;

/**
 * @brief  Captures one frame, encodes it to JPEG and stores it on the SD card.
 *
 * Updates g_imaging_stats with the outcome. On any failure the function
 * returns early and no image is kept.
 *
 * @param frame Buffer the camera DMA writes the raw frame into.
 */
static void camera_capture_single(uint32_t *frame)
{
	const img_jpeg_cfg_t cfg = { IMG_CAPTURE_WIDTH, IMG_CAPTURE_HEIGHT, IMG_FLIGHT_QUALITY };
	img_source_t src = { .ram = (const uint8_t *)frame, .swap_bytes = IMG_CAPTURE_SWAP };
	img_jpeg_result_t res;

	// Grab a frame from the camera
	if (img_capture_frame(frame, 0) != 0) {
		g_imaging_stats.capture_failed++;
		return;
	}

	// Reserve a file on the SD card
	if (img_store_begin(&s_slot) != STORE_OK) {
		g_imaging_stats.store_failed++;
		return;
	}

	// Encode straight into the open file; discard the slot if it fails
	img_status_t st = img_jpeg_encode(&cfg, &src, &s_slot.file, &res);
	if (st != IMG_OK) {
		img_store_abort(&s_slot);
		g_imaging_stats.encode_failed++;
		g_imaging_stats.last_error = st;
		return;
	}

	// Finalise the file
	if (img_store_commit(&s_slot) != STORE_OK) {
		g_imaging_stats.store_failed++;
		return;
	}

	g_imaging_stats.captured++;
	g_imaging_stats.last_id = s_slot.id;
	g_imaging_stats.last_result = res;
}

/**
 * @brief  Flight mode: captures and stores an image every IMG_CAPTURE_PERIOD_MS.
 *
 * Never returns.
 *
 * @param frame Buffer the camera DMA writes the raw frame into.
 */
void camera_capture_continuous(uint32_t *frame)
{
	for (;;) {
		camera_capture_single(frame);
		osDelay(IMG_CAPTURE_PERIOD_MS);
	}
}

/**
 * @brief  USB mode: serves the dashboard over USB CDC.
 *
 * When a valid 8-byte command arrives ([0xFF, colour, resolution, pattern,
 * AEC high, AEC low, gain, 0xFE]), configures the camera, captures an image
 * and streams it to the dashboard in chunks of 20 rows. Otherwise sends the
 * INA219 power reading periodically.
 *
 * Never returns.
 */
void camera_dashboard_operate()
{
	for(;;)
	{
	  if (recvDone == 1)
	  {

		  // Check if length of received matches expected
		  if (recvLen != 8) continue;

		  // Check start and end byte
		  if ((UserRxBufferHS[0] != 0xFF) || (UserRxBufferHS[7] != 0xFE)) continue;

		  // Extract data
		  uint8_t colour_mode = UserRxBufferHS[1];
		  uint8_t resolution_mode = UserRxBufferHS[2];
		  uint8_t test_pattern_mode = UserRxBufferHS[3];
		  uint16_t aec_value = (UserRxBufferHS[4] << 8) + UserRxBufferHS[5];
		  uint8_t gain_value = UserRxBufferHS[6];

		  // resolution_mode 0 = VGA, otherwise QVGA
		  uint32_t image_size = (resolution_mode == 0) ? VGA_IMAGE_SIZE_BYTES : QVGA_IMAGE_SIZE_BYTES;
		  uint32_t row_size = (resolution_mode == 0) ? VGA_ROW_BYTES: QVGA_ROW_BYTES;
		  uint32_t buffer_size = row_size * 20; // 20 rows at a time

		  // Setup Camera if config is different
		  camera_config = (colour_mode) |
		                  (resolution_mode << 2) |
		                  (test_pattern_mode << 4) |
		                  ((uint32_t)aec_value << 7) |
		                  ((uint32_t)gain_value << 23);
		  if (camera_config != camera_config_prev)
		  {
			  HAL_GPIO_WritePin(DCMI_PWRDWN_GPIO_Port, DCMI_PWRDWN_Pin, GPIO_PIN_RESET);	// Power up
			  OV7670_Config(colour_mode, resolution_mode, test_pattern_mode, aec_value, gain_value);
			  osDelay(250); // Config pause time
			  camera_config_prev = camera_config;
		  }

		  // Block until ready for image capture
		  while (hdcmi.State != HAL_DCMI_STATE_READY);

		  // --- IMAGE CAPTURE ---
		  HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)pBuffer, image_size/4);	// Size in words

		  // Block until image captured
		  while (hdcmi.State != HAL_DCMI_STATE_READY);
		  HAL_DCMI_Stop(&hdcmi);
		  HAL_GPIO_WritePin(DCMI_PWRDWN_GPIO_Port, DCMI_PWRDWN_Pin, GPIO_PIN_SET);	// Power down

		  // Transmit each row
		  for (uint32_t offset = 0; offset < image_size; offset += buffer_size)
		  {
			  Dashboard_Transmit(IMAGE_PACKET_ID, (uint8_t *)pBuffer + offset, buffer_size);
		  }

		  recvDone = 0;	// Ready for the next command

	  }
	  else // Send CSA data
	  {
		  CSA_ReadAll(&csa);
		  uint16_t power_to_send = csa.power_mW;
		  Dashboard_Transmit(POWER_PACKET_ID, (uint8_t *)&power_to_send, sizeof(uint16_t));

		  // Short delay so the dashboard is readable
		  osDelay(100);
	  }
	}
}
