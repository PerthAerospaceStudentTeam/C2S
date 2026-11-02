/*
 * hcsa.h
 *
 *  Created on: Jun 19, 2026
 *      Author: felix
 */

#include "ina219.h"

/**
 * @brief  Executes setup functions in order.
 *
 * @param *cal Pointer to INA struct.
 * @param *cal Pointer to I2C HAL linked to the CSA e.g., hi2c2
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef CSA_Setup(CSA_HandleTypeDef *hcsa, I2C_HandleTypeDef *hi2c)
{
	hcsa->hi2c = hi2c;
	CSA_Calculate_Calibration(hcsa);
	CSA_Write_Calibration(hcsa);
	CSA_Read_Calibration(hcsa);
	return HAL_OK;
}

/**
 * @brief  Reads hcsa registers into struct
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef CSA_ReadAll(CSA_HandleTypeDef *hcsa)
{
	CSA_Read_Bus(hcsa);
	CSA_Read_Shunt(hcsa);
	CSA_Read_Power(hcsa);
	CSA_Read_Current(hcsa);
	CSA_Calculate_Draw(hcsa);
	return HAL_OK;
}

/**
 * @brief  Calculates the PG value according to gain and shunt resistance defined in config.h.
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef CSA_Calculate_Calibration(CSA_HandleTypeDef *hcsa)
{
	float v;

	switch (CSA_GAIN) // Sets voltage according to PG Bit Settings Table 4 https://www.ti.com/lit/ds/symlink/hcsa.pdf
	{
		case 1:
			v = 0.04;
			break;
		case 2:
			v = 0.08;
			break;
		case 4:
			v = 0.16;
			break;
		case 8:
			v = 0.32;
			break;
		default:
			return HAL_ERROR;
	}

	hcsa->current_lsb = v / SHUNT_RESISTANCE / (1 << 15);
	hcsa->calibration = (uint16_t)(0.04096 / (hcsa->current_lsb * SHUNT_RESISTANCE));

	return HAL_OK;

}

/**
 * @brief  Configures the hcsa bus voltage range, PGA gain, ADC resolution
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef CSA_Write_Calibration(CSA_HandleTypeDef *hcsa)
{

	if (hcsa->calibration == 0x00) { return HAL_ERROR; } // Not calibrated yet or error with calculation

	// Device did not acknowledge (NACK/AF occurred)
	if (HAL_I2C_IsDeviceReady((hcsa->hi2c), (CSA_ADDRESS << 1), 3, CSA_TIMEOUT) != HAL_OK) return HAL_ERROR;

	uint8_t tx_buffer[3];
	tx_buffer[0] = CSA_REG_CALIBRATION;
    tx_buffer[1] = (hcsa->calibration) >> 8;
    tx_buffer[2] = (hcsa->calibration) & 0xFF;

	return HAL_I2C_Master_Transmit((hcsa->hi2c), (CSA_ADDRESS << 1), tx_buffer, 3, CSA_TIMEOUT);

}

/**
 * @brief  Configures the hcsa bus voltage range, PGA gain, ADC resolution
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef CSA_Read_Calibration(CSA_HandleTypeDef *hcsa)
{

	// Device did not acknowledge (NACK/AF occurred)
	if (HAL_I2C_IsDeviceReady((hcsa->hi2c), (CSA_ADDRESS << 1), 3, CSA_TIMEOUT) != HAL_OK) return HAL_ERROR;

	uint8_t rx_buffer[2]; // First, Second byte

	if (HAL_I2C_Mem_Read((hcsa->hi2c), (CSA_ADDRESS << 1), CSA_REG_CALIBRATION, 1, rx_buffer, 2, CSA_TIMEOUT) != HAL_OK) return HAL_ERROR;

	return HAL_OK;

}

/**
 * @brief  Reads the Bus Voltage register
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef CSA_Read_Bus(CSA_HandleTypeDef *hcsa)
{

	// Device did not acknowledge (NACK/AF occurred)
	if (HAL_I2C_IsDeviceReady((hcsa->hi2c), (CSA_ADDRESS << 1), 3, CSA_TIMEOUT) != HAL_OK) return HAL_ERROR;

	uint8_t rx_buffer[2];

	if (HAL_I2C_Mem_Read(hcsa->hi2c, (CSA_ADDRESS << 1), CSA_REG_BUS_VOLTAGE, I2C_MEMADD_SIZE_8BIT, rx_buffer, 2, CSA_TIMEOUT) != HAL_OK) return HAL_ERROR;

	uint16_t raw = ((uint16_t)rx_buffer[0] << 8) | rx_buffer[1];

	if (raw & 0x0001) return HAL_ERROR;      // OVF: math overflow, reading not valid

	hcsa->bus_voltage = (raw >> 3) * 4;    // bits 15:3, 4 mV per LSB -> millivolts

	return HAL_OK;
}

/**
 * @brief  Reads the Shunt Voltage register
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef CSA_Read_Shunt(CSA_HandleTypeDef *hcsa)
{
	// Device did not acknowledge (NACK/AF occurred)
	if (HAL_I2C_IsDeviceReady((hcsa->hi2c), (CSA_ADDRESS << 1), 3, CSA_TIMEOUT) != HAL_OK) return HAL_ERROR;

	uint8_t rx_buffer[2]; // First, Second byte

	if (HAL_I2C_Mem_Read((hcsa->hi2c), (CSA_ADDRESS << 1), CSA_REG_SHUNT_VOLTAGE, 1, rx_buffer, 2, CSA_TIMEOUT) != HAL_OK) return HAL_ERROR;

	hcsa->shunt_voltage = (uint16_t)(rx_buffer[0] << 8) | rx_buffer[1];

	return HAL_OK;

}

/**
 * @brief  Reads the Power register
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef CSA_Read_Power(CSA_HandleTypeDef *hcsa)
{

	// Device did not acknowledge (NACK/AF occurred)
	if (HAL_I2C_IsDeviceReady((hcsa->hi2c), (CSA_ADDRESS << 1), 3, CSA_TIMEOUT) != HAL_OK) return HAL_ERROR;

	uint8_t rx_buffer[2]; // First, Second byte

	if (HAL_I2C_Mem_Read((hcsa->hi2c), (CSA_ADDRESS << 1), CSA_REG_POWER, 1, rx_buffer, 2, CSA_TIMEOUT) != HAL_OK) return HAL_ERROR;

	hcsa->power = (uint16_t)(rx_buffer[0] << 8) | rx_buffer[1];

	return HAL_OK;

}

/**
 * @brief  Reads the Current register
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef CSA_Read_Current(CSA_HandleTypeDef *hcsa)
{

	// Device did not acknowledge (NACK/AF occurred)
	if (HAL_I2C_IsDeviceReady((hcsa->hi2c), (CSA_ADDRESS << 1), 3, CSA_TIMEOUT) != HAL_OK) return HAL_ERROR;

	uint8_t rx_buffer[2]; // First, Second byte

	if (HAL_I2C_Mem_Read((hcsa->hi2c), (CSA_ADDRESS << 1), CSA_REG_CURRENT, 1, rx_buffer, 2, CSA_TIMEOUT) != HAL_OK) return HAL_ERROR;

	hcsa->current = (uint16_t)(rx_buffer[0] << 8) | rx_buffer[1];

	return HAL_OK;

}

/**
 * @brief  Calculates current and power draw from values in struct
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef CSA_Calculate_Draw(CSA_HandleTypeDef *hcsa)
{
	hcsa->current_mA = hcsa->current_lsb * hcsa->current * 1000;
	hcsa->power_mW = 20 * hcsa->current_lsb * hcsa->power * 1000;
	return HAL_OK;
}
