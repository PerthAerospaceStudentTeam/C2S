/**
 * @file    OV7670.c
 * @brief   OV7670 camera driver (SCCB register access and configuration).
 *
 * Originally by take-iwiw (2017/08/25).
 * Modified for STM32U575 GPDMA & D-Cache.
 */

#include "OV7670.h"

#include <stdio.h>
#include "main.h"
#include "ov7670_config.h"
#include "config.h"

#include "cmsis_os2.h"

/*** Defines ***/

/* Colour modes */
#define RGB565			0
#define GRAYSCALE		1
#define YCBCR			2

/* Resolution modes */
#define VGA_RESOLUTION	0
#define QVGA_RESOLUTION	1

/*** Static Variables ***/

static I2C_HandleTypeDef *sp_hi2c;          /**< I2C handle used for SCCB access. */
static void (*s_cbHsync)(uint32_t h);       /**< Registered HSYNC callback. */
static void (*s_cbVsync)(uint32_t v);       /**< Registered VSYNC callback. */
static uint32_t s_currentV;                 /**< Running frame counter. */

/*** Static Functions ***/

/**
 * @brief  Write a single OV7670 register over SCCB.
 * @param  regAddr  Register address.
 * @param  data     Value to write.
 * @return HAL status of the I2C transfer.
 */
static HAL_StatusTypeDef OV7670_Write_Register(uint8_t regAddr, uint8_t data)
{
	HAL_StatusTypeDef ret;
	do {
		ret = HAL_I2C_Mem_Write(sp_hi2c, SLAVE_ADDR, regAddr, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
	} while (ret != HAL_OK && 0);
	return ret;
}

/**
 * @brief  Read a single OV7670 register over SCCB.
 * @param  regAddr  Register address.
 * @param  data     Pointer to store the value read.
 * @return HAL status of the I2C transfers.
 */
static HAL_StatusTypeDef OV7670_Read_Register(uint8_t regAddr, uint8_t *data)
{
	HAL_StatusTypeDef ret;

	if (sp_hi2c == NULL || data == NULL) return HAL_ERROR;

	/* SCCB needs a STOP between the write and read phases, so
	 * HAL_I2C_Mem_Read() (repeated start) can't be used. */
	ret = HAL_I2C_Master_Transmit(sp_hi2c, SLAVE_ADDR, &regAddr, 1, 100);
	if (ret != HAL_OK) return ret;

	return HAL_I2C_Master_Receive(sp_hi2c, SLAVE_ADDR, data, 1, 100);
}

/*** Public Functions ***/

/**
 * @brief  Read the device ID register (0x0B).
 * @param  buffer  Buffer to store the ID byte.
 * @return HAL status of the read.
 */
HAL_StatusTypeDef OV7670_Read_DevID(uint8_t buffer[])
{
	if (OV7670_Read_Register(0x0A, &buffer[1]) != HAL_OK) return HAL_ERROR; // PID
	if (OV7670_Read_Register(0x0B, &buffer[0]) != HAL_OK) return HAL_ERROR; // VER
	return HAL_OK;
}

/**
 * @brief  Reset the sensor and store the I2C handle.
 * @param  p_hdcmi      DCMI handle (unused).
 * @param  p_hdma_dcmi  DMA handle (unused).
 * @param  p_hi2c       I2C handle connected to the sensor.
 * @return HAL_OK.
 * @note   Calls osDelay(), so the kernel must be running.
 */
HAL_StatusTypeDef OV7670_Init(DCMI_HandleTypeDef *p_hdcmi, DMA_HandleTypeDef *p_hdma_dcmi, I2C_HandleTypeDef *p_hi2c)
{
	(void)p_hdcmi;
	(void)p_hdma_dcmi;
	sp_hi2c = p_hi2c;

	HAL_GPIO_WritePin(DCMI_RESET_GPIO_Port, DCMI_RESET_Pin, GPIO_PIN_RESET);
	osDelay(100);
	HAL_GPIO_WritePin(DCMI_RESET_GPIO_Port, DCMI_RESET_Pin, GPIO_PIN_SET);
	osDelay(100);

	OV7670_Write_Register(0x12, 0x80);  // RESET
	osDelay(30);

	uint8_t buffer[4];
	OV7670_Read_DevID(buffer);

	return HAL_OK;
}

/**
 * @brief  Configure the sensor for a given colour format, resolution and exposure.
 * @param  colour_mode        Colour format: 0 = RGB565, 1 = Grayscale, 2 = YCbCr.
 * @param  resolution_mode    Resolution: 0 = VGA, 1 = QVGA.
 * @param  test_pattern_mode  8-bar test pattern: 0 = off, 1 = on.
 * @param  aec_value          Exposure (AEC) value, 16-bit.
 * @param  gain_value         Value written to the GAIN register.
 * @return HAL_OK.
 */
HAL_StatusTypeDef OV7670_Config(uint8_t colour_mode, uint8_t resolution_mode, uint8_t test_pattern_mode, uint16_t aec_value, uint8_t gain_value)
{
	// Clear Registers
	OV7670_Write_Register(0x12, 0x80);

	// Resolution Configuration
	const uint8_t (*Resolution_Config)[2] = NULL;
	if (resolution_mode == QVGA_RESOLUTION) { Resolution_Config = QVGA_Config; }
	else if (resolution_mode == VGA_RESOLUTION) { Resolution_Config = VGA_Config; }

	// Colour Configuration
	const uint8_t (*Colour_Config)[2] = NULL;
	if (colour_mode == RGB565) { Colour_Config = RGB565_Config; }
	else if (colour_mode == GRAYSCALE) { Colour_Config = Grayscale_Config; }
	else if (colour_mode == YCBCR) { Colour_Config = YCbCr_Config; }

	// Apply System-wide Configuration
	for (int i = 0; System_Config[i][0] != REG_EOF; i++) {
		OV7670_Write_Register(System_Config[i][0], System_Config[i][1]);
	}

	// Apply COM7 Register
	uint8_t COM7_Val = (16 * resolution_mode);
	if (test_pattern_mode) { COM7_Val += 2; }
	if (colour_mode == RGB565) { COM7_Val += 4; }
	else if (colour_mode == YCBCR || colour_mode == GRAYSCALE) { COM7_Val += 0; }
	OV7670_Write_Register(OV7670_COM7_ADDRESS, COM7_Val);

	// Apply Resolution Configuration
	if (Resolution_Config != NULL) {
		for (int i = 0; Resolution_Config[i][0] != REG_EOF; i++) {
			OV7670_Write_Register(Resolution_Config[i][0], Resolution_Config[i][1]);
		}
	}

	// Apply Colour Configuration
	if (Colour_Config != NULL) {
		for (int i = 0; Colour_Config[i][0] != REG_EOF; i++) {
			OV7670_Write_Register(Colour_Config[i][0], Colour_Config[i][1]);
		}
	}

	// Apply Test Pattern (if applicable)
	if (test_pattern_mode) {
		for (int i = 0; Test_Config[i][0] != REG_EOF; i++) {
			OV7670_Write_Register(Test_Config[i][0], Test_Config[i][1]);
		}
	}

	// Set AEC Register (Other bits of AEC registers are default set to 0, so can directly write AEC values without overwriting)
	uint8_t aechh_register_value = (aec_value >> 10) & 0x3F;
	uint8_t aech_register_value = (aec_value >> 2) & 0xFF;
	uint8_t com1_register_value = aec_value & 0x03;
	OV7670_Write_Register(OV7670_AECHH_ADDRESS, aechh_register_value); 	// Register bits [5:0] => AEC [15:10]
	OV7670_Write_Register(OV7670_AECH_ADDRESS, aech_register_value); 	// Register bits [7:0] => AEC [9:2]
	OV7670_Write_Register(OV7670_COM1_ADDRESS, com1_register_value); 	// Register bits [1:0] => AEC [1:0]

	// Set Gain Register
	OV7670_Write_Register(OV7670_GAIN_ADDRESS, gain_value);

	return HAL_OK;
}

/**
 * @brief  Register HSYNC and VSYNC callbacks.
 * @param  cbHsync  Callback for line events (currently not called).
 * @param  cbVsync  Callback for frame events, receives the frame counter.
 */
void OV7670_ConfigisterCallback(void (*cbHsync)(uint32_t h), void (*cbVsync)(uint32_t v))
{
	s_cbHsync = cbHsync;
	s_cbVsync = cbVsync;
}

/*** HAL DCMI Callbacks ***/

/**
 * @brief  DCMI frame-complete callback: calls the VSYNC callback and increments the frame counter.
 * @param  hdcmi  DCMI handle (unused).
 */
void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
	if (s_cbVsync) s_cbVsync(s_currentV);

	s_currentV++;
}

/**
 * @brief  DCMI line-event callback (not used).
 * @param  hdcmi  DCMI handle (unused).
 */
void HAL_DCMI_LineEventCallback(DCMI_HandleTypeDef *hdcmi)
{
	//
}

/**
 * @brief  DCMI VSYNC-event callback (intentionally blank, GPDMA handles transfers).
 * @param  hdcmi  DCMI handle (unused).
 */
void HAL_DCMI_VsyncEventCallback(DCMI_HandleTypeDef *hdcmi)
{
	// Intentionally blank - GPDMA handles transfers automatically
}
