/*
 * rgb_led.c
 *
 *  Created on: 25 Sept 2026
 *      Author: felix
 */

#include "rgb_led.h"

static HAL_StatusTypeDef RGB_SendBit(uint8_t bit);

extern TIM_HandleTypeDef RGB_TIM;

/**
 * @brief  Executes setup functions in order.
 */
void RGB_Reset()
{
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); // Needs to start high to have falling edge

	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

	HAL_Delay(1); // Wait at least 80us

	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

/**
 * @brief  Executes setup functions in order.
 *
 * @param red Brightness value of red channel (0-255)
 * @param blue Brightness value of blue channel (0-255)
 * @param green Brightness value of green channel (0-255)
 */
void RGB_SetColour(uint8_t red, uint8_t green, uint8_t blue)
{
    RGB_Reset();

    // Send Green (8 bits)
    for (int i = 7; i >= 0; i--) {
        RGB_SendBit((green >> i) & 0x01);
    }
    // Send Red (8 bits)
    for (int i = 7; i >= 0; i--) {
        RGB_SendBit((red >> i) & 0x01);
    }
    // Send Blue (8 bits)
    for (int i = 7; i >= 0; i--) {
        RGB_SendBit((blue >> i) & 0x01);
    }
}

/**
 * @brief Sends a single bit (0/1) to the RGB LED
 *
 * @param bit Data to be send 0/1
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for bit error
 */
static HAL_StatusTypeDef RGB_SendBit(uint8_t bit)
{
	// Sanity Check
	if ((bit != 0) && (bit != 1)) return HAL_ERROR;

	// Start rising edge
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

	// Start timer
	RGB_TIM.Instance->RCR = bit;
	HAL_TIM_Base_Start_IT(&RGB_TIM);

	// Wait until timer stopped
	while (HAL_TIM_Base_GetState(&RGB_TIM) == HAL_TIM_STATE_BUSY);

	HAL_Delay(1); // Pause between bits

	return HAL_OK;
}

/**
 * @brief  Re-definition of HAL function for timer interrupt
 *
 * @param *htim HAL timer pointer
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	// Sanity Check
	if (htim->Instance == RGB_TIM.Instance)
	{
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

		HAL_TIM_Base_Stop_IT(htim);

		if (htim->Instance->RCR != 3) // If not in low level time currently
		{
			RGB_TIM.Instance->RCR = 3;

			HAL_TIM_Base_Start_IT(&RGB_TIM);
		}


	}
}
