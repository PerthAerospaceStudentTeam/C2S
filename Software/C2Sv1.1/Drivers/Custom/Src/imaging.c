/*
 * imaging.c
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 *
 *  Modified on: 7/11/2026
 *  	Author: felix
 *
 */

#include "imaging.h"

#include "microSD.h"
#include "cmsis_os2.h"
#include "ina219.h"
#include "usbd_cdc_if.h"
#include "dashboard.h"
#include "ov7670.h"
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"

// --- Camera settings used for flight/self-test captures ---
#define CAM_RGB565			0
#define CAM_QVGA			1
#define CAM_SETTLE_MS		300u
#define CAM_FRAME_TIMEOUT	1000u

#define CAM_AEC_VALUE		512u
#define CAM_GAIN_VALUE		0xA0u

volatile imaging_stats_t g_imaging_stats;	// Running capture/store/encode counters
volatile selftest_report_t g_img_selftest;	// Results of the last self-test
static img_store_slot_t s_slot;				// Current SD card slot being written
static FIL s_file;							// File used by the self-test

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

// --- IMAGE BUFFER ---
volatile imaging_req_t g_imaging_req;

/**
 * @brief  Waits for the DCMI to return to the ready state.
 *
 * @param timeout_ms Maximum time to wait in milliseconds.
 *
 * @retval 0 if ready, -1 on timeout
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

/**
 * @brief  Powers up and configures the camera, captures one QVGA RGB565 frame, then powers it down.
 *
 * @param *frame Buffer the camera DMA writes the raw frame into (IMG_CAPTURE_BYTES).
 * @param test_pattern 1 for the camera colour bar test pattern, 0 for a live image.
 *
 * @retval 0 on success, -1 on failure or timeout
 */
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
	//HAL_GPIO_WritePin(DCMI_PWRDWN_GPIO_Port, DCMI_PWRDWN_Pin, GPIO_PIN_SET);
	return ret;
}

/**
 * @brief  Writes a captured frame to an open file, as JPEG or raw RGB565 depending on COMPRESSION_MODE.
 *
 * @param *frame Captured raw frame (IMG_CAPTURE_BYTES).
 * @param quality JPEG quality, ignored when compression is disabled.
 * @param *dst Pointer to the open destination file.
 * @param *res Pointer to the result (bytes written and time taken).
 *
 * @retval IMG_OK on success, otherwise the img_status_t error code
 */
static img_status_t write_frame(const uint32_t *frame, uint8_t quality, FIL *dst, img_jpeg_result_t *res)
{
#if COMPRESSION_MODE == COMPRESSION_ENABLE
	const img_jpeg_cfg_t cfg = { IMG_CAPTURE_WIDTH, IMG_CAPTURE_HEIGHT, quality };
	img_source_t src = { .ram = (const uint8_t *)frame, .swap_bytes = IMG_CAPTURE_SWAP };

	return img_jpeg_encode(&cfg, &src, dst, res);
#else
	(void)quality;
	UINT written = 0;
	uint32_t start = HAL_GetTick();

	FRESULT fr = f_write(dst, frame, IMG_CAPTURE_BYTES, &written);

	res->encode_ms  = HAL_GetTick() - start;
	res->jpeg_bytes = written;
	return (fr == FR_OK && written == IMG_CAPTURE_BYTES) ? IMG_OK : IMG_ERR_WRITE;
#endif
}

/**
 * @brief  Captures one frame and stores it on the SD card (JPEG or raw, see COMPRESSION_MODE).
 *
 * Updates g_imaging_stats with the outcome. On any failure the function
 * returns early and no image is kept.
 *
 * @param *frame Buffer the camera DMA writes the raw frame into.
 */
static void camera_capture_single(uint32_t *frame)
{
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

	// Write straight into the open file; discard the slot if it fails
	img_status_t st = write_frame(frame, IMG_FLIGHT_QUALITY, &s_slot.file, &res);
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

	// Copy data to request struct
	g_imaging_req.idx = g_imaging_stats.last_id;
}

/**
 * @brief  Flight mode: captures and stores an image every IMG_CAPTURE_PERIOD_MS.
 *
 * Never returns.
 *
 * @param *frame Buffer the camera DMA writes the raw frame into.
 */
void camera_capture_continuous(uint32_t *frame)
{
	// Setup
	g_imaging_req.buf = 0;
	g_imaging_req.idx = 0;
	g_imaging_req.src = IMG_SRC_NIL;
	g_imaging_req.timestamp = 0;

	for (;;) {

		// Take an image if valid request
		if ((g_imaging_req.buf != 0) && (g_imaging_req.src != IMG_SRC_NIL))
		{
			camera_capture_single(frame);
			g_imaging_req.timestamp = xTaskGetTickCount();
			g_imaging_req.buf = g_imaging_req.buf - 1;
			if (g_imaging_req.buf == 0) g_imaging_req.src = IMG_SRC_NIL;
		}

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
		  HAL_GPIO_WritePin(DCMI_PWRDWN_GPIO_Port, DCMI_PWRDWN_Pin, GPIO_PIN_RESET);	// Power up
		  camera_config = (colour_mode) |
		                  (resolution_mode << 2) |
		                  (test_pattern_mode << 4) |
		                  ((uint32_t)aec_value << 7) |
		                  ((uint32_t)gain_value << 23);
		  if (camera_config != camera_config_prev)
		  {
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

/**
 * @brief  Captures one self-test image and writes it to a named file on the SD card.
 *
 * @param *name File name to write, e.g., "BARS" IMG_FILE_EXT
 * @param *frame Buffer the camera DMA writes the raw frame into.
 * @param test_pattern 1 for the colour bar test pattern, 0 for a live image.
 * @param *tc Pointer to the report entry to fill.
 */
static void run_case(const char *name, uint32_t *frame, uint8_t test_pattern, volatile selftest_case_t *tc)
{
	img_jpeg_result_t res = {0};

	tc->captured = (img_capture_frame(frame, test_pattern) == 0);
	if (!tc->captured) {
		return;
	}

	img_status_t st = IMG_ERR_WRITE;
	if (f_open(&s_file, name, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
		st = write_frame(frame, IMG_SELFTEST_QUALITY, &s_file, &res);
		if (f_close(&s_file) != FR_OK && st == IMG_OK) {
			st = IMG_ERR_WRITE;
		}
	}

	tc->status = st;
	tc->result = res;
}

/**
 * @brief  Runs the camera/storage self-test: saves BARS and LIVE images (test pattern and live) with IMG_FILE_EXT.
 *
 * Results are stored in g_img_selftest. Does nothing if the store is not ready.
 *
 * @param *frame Buffer the camera DMA writes the raw frame into.
 */
void img_selftest_run(uint32_t *frame)
{
	g_img_selftest.store_ready = img_store_ready();
	if (!g_img_selftest.store_ready) {
		return;
	}

	run_case("BARS" IMG_FILE_EXT, frame, 1, &g_img_selftest.bars);
	run_case("LIVE" IMG_FILE_EXT, frame, 0, &g_img_selftest.live);
}
