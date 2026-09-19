/*
 * ina219.h
 *
 *  Created on: Jun 19, 2026
 *      Author: felix
 */

#ifndef CUSTOM_INC_PERIPHERALS_INA219_H_
#define CUSTOM_INC_PERIPHERALS_INA219_H_

#include "config.h"

/**
 * @brief chip register definition
 */
#define INA219_REG_CONF                 0x00        /**< configuration register */
#define INA219_REG_SHUNT_VOLTAGE        0x01        /**< shunt voltage register */
#define INA219_REG_BUS_VOLTAGE          0x02        /**< bus voltage register */
#define INA219_REG_POWER                0x03        /**< power register */
#define INA219_REG_CURRENT              0x04        /**< current register */
#define INA219_REG_CALIBRATION          0x05        /**< calibration register */

//#define INA219_ADDRESS_WRITE (INA219_ADDRESS << 1)			// Force LSB to 0
//#define INA219_ADDRESS_READ  ((INA219_ADDRESS << 1) | 1U)	// Force LSB to 1

#define INA219_TIMEOUT 1000

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
}INA219_HandleTypeDef;

HAL_StatusTypeDef INA219_Setup(INA219_HandleTypeDef *INA219, I2C_HandleTypeDef *hi2c_tmp);
HAL_StatusTypeDef INA219_ReadAll(INA219_HandleTypeDef *INA219);
HAL_StatusTypeDef INA219_Calculate_Calibration(INA219_HandleTypeDef *INA219);
HAL_StatusTypeDef INA219_Write_Calibration(INA219_HandleTypeDef *INA219);
HAL_StatusTypeDef INA219_Read_Calibration(INA219_HandleTypeDef *INA219);
HAL_StatusTypeDef INA219_Read_Bus(INA219_HandleTypeDef *INA219);
HAL_StatusTypeDef INA219_Read_Shunt(INA219_HandleTypeDef *INA219);
HAL_StatusTypeDef INA219_Read_Power(INA219_HandleTypeDef *INA219);
HAL_StatusTypeDef INA219_Read_Current(INA219_HandleTypeDef *INA219);
HAL_StatusTypeDef INA219_Calculate_Draw(INA219_HandleTypeDef *INA219);

#endif /* CUSTOM_INC_PERIPHERALS_INA219_H_ */
