/*
 * ina219.h
 *
 *  Created on: Jun 19, 2026
 *      Author: felix
 */

#ifndef CUSTOM_INC_PERIPHERALS_CSA_H_
#define CUSTOM_INC_PERIPHERALS_CSA_H_

#include "config.h"

/**
 * @brief chip register definition
 */
#define CSA_REG_CONF                 0x00        /**< configuration register */
#define CSA_REG_SHUNT_VOLTAGE        0x01        /**< shunt voltage register */
#define CSA_REG_BUS_VOLTAGE          0x02        /**< bus voltage register */
#define CSA_REG_POWER                0x03        /**< power register */
#define CSA_REG_CURRENT              0x04        /**< current register */
#define CSA_REG_CALIBRATION          0x05        /**< calibration register */

//#define CSA_ADDRESS_WRITE (CSA_ADDRESS << 1)			// Force LSB to 0
//#define CSA_ADDRESS_READ  ((CSA_ADDRESS << 1) | 1U)	// Force LSB to 1

#define CSA_TIMEOUT 1000

typedef struct {
	I2C_HandleTypeDef		*hi2c;
	float					current_lsb;
	uint16_t 				calibration;
	uint16_t 				shunt_voltage;	// 10uV increments e.g., 50 = 500uV
	uint16_t 				bus_voltage;	// 4mV increments e.g., 50 = 200mV
	uint16_t 				power;			// 20*current_lsb increments
	uint16_t 				current;		// current_lsb increments
	float					current_mA;
	float					power_mW;
}CSA_HandleTypeDef;

HAL_StatusTypeDef CSA_Setup(CSA_HandleTypeDef *INA219, I2C_HandleTypeDef *hi2c_tmp);
HAL_StatusTypeDef CSA_ReadAll(CSA_HandleTypeDef *INA219);
HAL_StatusTypeDef CSA_Calculate_Calibration(CSA_HandleTypeDef *INA219);
HAL_StatusTypeDef CSA_Write_Calibration(CSA_HandleTypeDef *INA219);
HAL_StatusTypeDef CSA_Read_Calibration(CSA_HandleTypeDef *INA219);
HAL_StatusTypeDef CSA_Read_Bus(CSA_HandleTypeDef *INA219);
HAL_StatusTypeDef CSA_Read_Shunt(CSA_HandleTypeDef *INA219);
HAL_StatusTypeDef CSA_Read_Power(CSA_HandleTypeDef *INA219);
HAL_StatusTypeDef CSA_Read_Current(CSA_HandleTypeDef *INA219);
HAL_StatusTypeDef CSA_Calculate_Draw(CSA_HandleTypeDef *INA219);

#endif /* CUSTOM_INC_PERIPHERALS_CSA_H_ */
