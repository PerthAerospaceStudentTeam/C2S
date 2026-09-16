/*
 * led.c
 *
 *  Created on: Dec 12, 2025
 *      Author: felix
 */

#include "led.h"

/**
 * Toggles the MCU LED
 */
void LED_TOGGLE_MCU()
{
	HAL_GPIO_TogglePin(MCU_LED_2_GPIO_Port, MCU_LED_2_Pin);
}

/**
 * Configures the MCU LED
 */
void LED_WRITE_MCU(uint16_t PinState)
{
	HAL_GPIO_WritePin(MCU_LED_2_GPIO_Port, MCU_LED_2_Pin, PinState);
}

/**
 * Toggles the Sensor LED
 */
void LED_TOGGLE_SENSOR()
{
    HAL_GPIO_TogglePin(SENSOR_LED_2_GPIO_Port, SENSOR_LED_2_Pin);
}

/**
 * Configures the Sensor LED
 */
void LED_WRITE_SENSOR(uint16_t PinState)
{
    HAL_GPIO_WritePin(SENSOR_LED_2_GPIO_Port, SENSOR_LED_2_Pin, PinState ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * Toggles the Read LED
 */
void LED_TOGGLE_READ()
{
    HAL_GPIO_TogglePin(READ_LED_2_GPIO_Port, READ_LED_2_Pin);
}

/**
 * Configures the Read LED
 */
void LED_WRITE_READ(uint16_t PinState)
{
    HAL_GPIO_WritePin(READ_LED_2_GPIO_Port, READ_LED_2_Pin, PinState ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * Toggles the Write LED
 */
void LED_TOGGLE_WRITE()
{
    HAL_GPIO_TogglePin(WRITE_LED_2_GPIO_Port, WRITE_LED_2_Pin);
}

/**
 * Configures the Write LED
 */
void LED_WRITE_WRITE(uint16_t PinState)
{
    HAL_GPIO_WritePin(WRITE_LED_2_GPIO_Port, WRITE_LED_2_Pin, PinState ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * Toggles the Debug LED
 */
void LED_TOGGLE_DEBUG()
{
	HAL_GPIO_TogglePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin);
}

/**
 * Configures the Debug LED
 */
void LED_WRITE_DEBUG(uint16_t PinState)
{
	HAL_GPIO_WritePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin, PinState ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


/**
 * Set Anode of LEDs to LOW (GND)
 */
void LED_CONFIGURE_GND()
{
  HAL_GPIO_WritePin(MCU_LED_1_GPIO_Port, MCU_LED_1_Pin, 0);
  HAL_GPIO_WritePin(READ_LED_1_GPIO_Port, READ_LED_1_Pin, 0);
  HAL_GPIO_WritePin(WRITE_LED_1_GPIO_Port, WRITE_LED_1_Pin, 0);
  HAL_GPIO_WritePin(SENSOR_LED_1_GPIO_Port, SENSOR_LED_1_Pin, 0);
}

/**
 * Uses LEDs to represent 4-bit value
 *
 * @param value The 4-bit number
 */
void LED_SyncEncoder(volatile uint8_t value)
{
	LED_WRITE_SENSOR((value >> 0) & 0x01); // Bit 0
	LED_WRITE_MCU((value >> 1) & 0x01);    // Bit 1
	LED_WRITE_READ((value >> 2) & 0x01);   // Bit 2
	LED_WRITE_WRITE((value >> 3) & 0x01);  // Bit 3
}
