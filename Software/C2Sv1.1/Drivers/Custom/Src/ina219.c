/*
 * ina219.h
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
HAL_StatusTypeDef INA219_Setup(INA219_HandleTypeDef *INA219, I2C_HandleTypeDef *hi2c_tmp)
{
	INA219->hi2c = hi2c_tmp;
	INA219_Calculate_Calibration(INA219);
	INA219_Write_Calibration(INA219);
	INA219_Read_Calibration(INA219);

	return HAL_OK;
}

/**
 * @brief  Reads INA219 registers into struct
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef INA219_ReadAll(INA219_HandleTypeDef *INA219)
{
	INA219_Read_Bus(INA219);
	INA219_Read_Shunt(INA219);
	INA219_Read_Power(INA219);
	INA219_Read_Current(INA219);

	INA219_Calculate_Draw(INA219);

	return HAL_OK;
}

/**
 * @brief  Calculates the PG value according to gain and shunt resistance defined in config.h.
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef INA219_Calculate_Calibration(INA219_HandleTypeDef *INA219)
{
	float v;

	switch (INA219_GAIN) // Sets voltage according to PG Bit Settings Table 4 https://www.ti.com/lit/ds/symlink/ina219.pdf
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

	INA219->current_lsb = v / SHUNT_RESISTANCE / (1 << 15);
	INA219->calibration = (uint16_t)(0.04096 / (INA219->current_lsb * SHUNT_RESISTANCE));

	return HAL_OK;

}

/**
 * @brief  Configures the INA219 bus voltage range, PGA gain, ADC resolution
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef INA219_Write_Calibration(INA219_HandleTypeDef *INA219)
{

	if (INA219->calibration == 0x00) { return HAL_ERROR; } // Not calibrated yet or error with calculation

	if (HAL_I2C_IsDeviceReady((INA219->hi2c), (INA219_ADDRESS << 1), 3, INA219_TIMEOUT) != HAL_OK) {
	    // Device did not acknowledge (NACK/AF occurred)
		return HAL_ERROR;
	}

	// Prepare data
	uint8_t tx_buffer[3];
	tx_buffer[0] = INA219_REG_CALIBRATION;
    tx_buffer[1] = (INA219->calibration) >> 8;
    tx_buffer[2] = (INA219->calibration) & 0xFF;

	return HAL_I2C_Master_Transmit((INA219->hi2c), (INA219_ADDRESS << 1), tx_buffer, 3, INA219_TIMEOUT);

}

/**
 * @brief  Configures the INA219 bus voltage range, PGA gain, ADC resolution
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef INA219_Read_Calibration(INA219_HandleTypeDef *INA219)
{

	if (HAL_I2C_IsDeviceReady((INA219->hi2c), (INA219_ADDRESS << 1), 3, INA219_TIMEOUT) != HAL_OK) {
	    // Device did not acknowledge (NACK/AF occurred)
		return HAL_ERROR;
	}

	// Prepare data
	uint8_t rx_buffer[2]; // First, Second byte

	if (HAL_I2C_Mem_Read((INA219->hi2c), (INA219_ADDRESS << 1), INA219_REG_CALIBRATION, 1, rx_buffer, 2, INA219_TIMEOUT) != HAL_OK)
	{
		return HAL_ERROR;
	}



	// TBD what to do with received value
	// ...

	return HAL_OK;

}

/**
 * @brief  Reads the Bus Voltage register
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef INA219_Read_Bus(INA219_HandleTypeDef *INA219)
{

	if (HAL_I2C_IsDeviceReady((INA219->hi2c), (INA219_ADDRESS << 1), 3, INA219_TIMEOUT) != HAL_OK) {
	    // Device did not acknowledge (NACK/AF occurred)
		return HAL_ERROR;
	}

	// Prepare data
	uint8_t rx_buffer[2]; // First, Second byte

	if (HAL_I2C_Mem_Read((INA219->hi2c), (INA219_ADDRESS << 1), INA219_REG_BUS_VOLTAGE, 1, rx_buffer, 2, INA219_TIMEOUT) != HAL_OK)
	{
		return HAL_ERROR;
	}

	INA219->bus_voltage = (uint16_t)(rx_buffer[0] << 8) | rx_buffer[1];

	return HAL_OK;

}

/**
 * @brief  Reads the Shunt Voltage register
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef INA219_Read_Shunt(INA219_HandleTypeDef *INA219)
{

	if (HAL_I2C_IsDeviceReady((INA219->hi2c), (INA219_ADDRESS << 1), 3, INA219_TIMEOUT) != HAL_OK) {
	    // Device did not acknowledge (NACK/AF occurred)
		return HAL_ERROR;
	}

	// Prepare data
	uint8_t rx_buffer[2]; // First, Second byte

	if (HAL_I2C_Mem_Read((INA219->hi2c), (INA219_ADDRESS << 1), INA219_REG_SHUNT_VOLTAGE, 1, rx_buffer, 2, INA219_TIMEOUT) != HAL_OK)
	{
		return HAL_ERROR;
	}

	INA219->shunt_voltage = (uint16_t)(rx_buffer[0] << 8) | rx_buffer[1];

	return HAL_OK;

}

/**
 * @brief  Reads the Power register
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef INA219_Read_Power(INA219_HandleTypeDef *INA219)
{

	if (HAL_I2C_IsDeviceReady((INA219->hi2c), (INA219_ADDRESS << 1), 3, INA219_TIMEOUT) != HAL_OK) {
	    // Device did not acknowledge (NACK/AF occurred)
		return HAL_ERROR;
	}

	// Prepare data
	uint8_t rx_buffer[2]; // First, Second byte

	if (HAL_I2C_Mem_Read((INA219->hi2c), (INA219_ADDRESS << 1), INA219_REG_POWER, 1, rx_buffer, 2, INA219_TIMEOUT) != HAL_OK)
	{
		return HAL_ERROR;
	}

	INA219->power = (uint16_t)(rx_buffer[0] << 8) | rx_buffer[1];

	return HAL_OK;

}

/**
 * @brief  Reads the Current register
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef INA219_Read_Current(INA219_HandleTypeDef *INA219)
{

	if (HAL_I2C_IsDeviceReady((INA219->hi2c), (INA219_ADDRESS << 1), 3, INA219_TIMEOUT) != HAL_OK) {
	    // Device did not acknowledge (NACK/AF occurred)
		return HAL_ERROR;
	}

	// Prepare data
	uint8_t rx_buffer[2]; // First, Second byte

	if (HAL_I2C_Mem_Read((INA219->hi2c), (INA219_ADDRESS << 1), INA219_REG_CURRENT, 1, rx_buffer, 2, INA219_TIMEOUT) != HAL_OK)
	{
		return HAL_ERROR;
	}

	INA219->current = (uint16_t)(rx_buffer[0] << 8) | rx_buffer[1];

	return HAL_OK;

}

/**
 * @brief  Calculates current and power draw from values in struct
 *
 * @param *cal Pointer to INA struct.
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef INA219_Calculate_Draw(INA219_HandleTypeDef *INA219)
{
	INA219->current_mA = INA219->current_lsb * INA219->current * 1000;
	INA219->power_mW = 20 * INA219->current_lsb * INA219->power * 1000;

	return HAL_OK;
}
