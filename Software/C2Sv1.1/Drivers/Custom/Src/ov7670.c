/*
 * OV7670.c
 *
 * Created on: 2017/08/25
 * Author: take-iwiw
 * Modified for STM32U575 GPDMA & D-Cache
 */

#include "OV7670.h"
#include <stdio.h>
#include "main.h"
#include "ov7670_config.h"

/*** Internal Const Values, Macros ***/
#define OV7670_QVGA_WIDTH  320
#define OV7670_QVGA_HEIGHT 240

/*** External Variables ***/
extern volatile uint8_t FrameProcessed;
extern volatile uint32_t processedRows;

/*** Internal Static Variables ***/
static DCMI_HandleTypeDef *sp_hdcmi;
static DMA_HandleTypeDef  *sp_hdma_dcmi;
static I2C_HandleTypeDef  *sp_hi2c;
static uint32_t    s_destAddressForContinuousMode;
static uint32_t    s_bufferLengthBytes;
static void (* s_cbHsync)(uint32_t h);
static void (* s_cbVsync)(uint32_t v);
static uint32_t s_currentH;
static uint32_t s_currentV;

/*** Internal Function Declarations ***/
static RET OV7670_Write_Register(uint8_t regAddr, uint8_t data);
static RET OV7670_Read_Register(uint8_t regAddr, uint8_t *data);

/*** Internal Function Defines ***/
static RET OV7670_Write_Register(uint8_t regAddr, uint8_t data)
{
  HAL_StatusTypeDef ret;
  do {
    ret = HAL_I2C_Mem_Write(sp_hi2c, SLAVE_ADDR, regAddr, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
  } while (ret != HAL_OK && 0);
  return ret;
}

static RET OV7670_Read_Register(uint8_t regAddr, uint8_t *data)
{
  HAL_StatusTypeDef ret;
  do {
    // HAL_I2C_Mem_Read doesn't work (because of SCCB protocol(doesn't have ack))? */
//    ret = HAL_I2C_Mem_Read(sp_hi2c, SLAVE_ADDR, regAddr, I2C_MEMADD_SIZE_8BIT, data, 1, 1000);
    ret = HAL_I2C_Master_Transmit(sp_hi2c, SLAVE_ADDR, &regAddr, 1, 100);
    ret |= HAL_I2C_Master_Receive(sp_hi2c, SLAVE_ADDR, data, 1, 100);
  } while (ret != HAL_OK && 0);
  return ret;
}

/*** External Function Defines ***/
RET OV7670_Init(DCMI_HandleTypeDef *p_hdcmi, DMA_HandleTypeDef *p_hdma_dcmi, I2C_HandleTypeDef *p_hi2c)
{
  sp_hdcmi     = p_hdcmi;
  sp_hdma_dcmi = p_hdma_dcmi;
  sp_hi2c      = p_hi2c;
  s_destAddressForContinuousMode = 0;

  // Total bytes for QVGA RGB565 (320 * 240 * 2)
  s_bufferLengthBytes = OV7670_QVGA_WIDTH * OV7670_QVGA_HEIGHT * 2;

  HAL_GPIO_WritePin(DCMI_RESET_GPIO_Port, DCMI_RESET_Pin, GPIO_PIN_RESET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(DCMI_RESET_GPIO_Port, DCMI_RESET_Pin, GPIO_PIN_SET);
  HAL_Delay(100);

  OV7670_Write_Register(0x12, 0x80);  // RESET
  HAL_Delay(30);

  uint8_t buffer[4];
  OV7670_Read_Register(0x0b, buffer);
  printf("[OV7670] dev id = %02X\n", buffer[0]);

  return RET_OK;
}

/**
  * @brief  Configures OV7670 sensor according to given input mode
  *
  * @param colour_mode: Defines colour format (0) RGB565; (1) Grayscale; (2) YCbCr
  * @param resolution_mode: Defines image resolution (0) VGA; (1) QVGA
  * @param test_pattern_mode: Displays the 8-bar test-pattern with the configured colour and resolution (0) No; (1) Yes.
  */
RET OV7670_Config(uint8_t colour_mode, uint8_t resolution_mode, uint8_t test_pattern_mode)
{
	// PM Declarations
	#define RGB565			0
	#define GRAYSCALE		1
	#define YCBCR			2
	#define VGA_RESOLUTION 	0
	#define QVGA_RESOLUTION 1

	// PV Declarations
//	extern const uint8_t RGB565_Config[][2];
//	extern const uint8_t Grayscale_Config[][2];
//	extern const uint8_t YCbCr_Config[][2];
//	extern const uint8_t Test_Config[][2];
//	extern const uint8_t QVGA_Config[][2];
//	extern const uint8_t VGA_Config[][2];

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

  return RET_OK;
}

void OV7670_ConfigisterCallback(void (*cbHsync)(uint32_t h), void (*cbVsync)(uint32_t v))
{
  s_cbHsync = cbHsync;
  s_cbVsync = cbVsync;
}

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
  if(s_cbVsync) s_cbVsync(s_currentV);

  s_currentV++;
  s_currentH = 0;

  // Update snapshot flag
  //FrameProcessed = 1;
}

void HAL_DCMI_LineEventCallback(DCMI_HandleTypeDef *hdcmi)
{
	//
}

void HAL_DCMI_VsyncEventCallback(DCMI_HandleTypeDef *hdcmi)
{
  // Intentionally blank - GPDMA handles transfers automatically
}
