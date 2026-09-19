/*
 * config.h
 *
 *  Created on: Feb 23, 2026
 *      Author: felix
 */

#ifndef CUSTOM_INC_CONFIG_H_
#define CUSTOM_INC_CONFIG_H_

#include "main.h"

#define NO_MODE 				0
#define USB_MODE 				1
#define SLAVE_MODE 				2

// ------------ USER INPUT STARTS HERE ------------

#define VERIFY_CFG 				0					// (0) Don't; (1) Do

#define OPERATIONAL_MODE 		USB_MODE

#define CSA_ACTIVE				1					// (0) Off; (1) On

// ------------ USER INPUT ENDS HERE ------------

// CSA
#define SHUNT_RESISTANCE 		1.6f
#define INA219_GAIN 			8 					// The dividing factor for the gain. Either 1, 2, 4, or 8.
#define INA219_ADDRESS 			0b1000000 			// A0 = GND, A1 = GND

// DCMI
#define DCMI_PWRDWN_Pin			DCMI_GPIO0_Pin
#define DCMI_PWRDWN_GPIO_Port	DCMI_GPIO0_GPIO_Port

#define DCMI_RESET_Pin			DCMI_GPIO2_Pin
#define DCMI_RESET_GPIO_Port	DCMI_GPIO2_GPIO_Port

#define LL_MAX_NODE_SIZE		38400

#define IMAGE_COLS				320
#define IMAGE_ROWS				240
#define DB_ROWS					4
#define DB_SIZE_BYTES 			(IMAGE_COLS*DB_ROWS*2) // 320 columns, 1 row, 2 bytes per pixel
#define DB_SIZE_WORDS 			(DB_SIZE_BYTES / 4)

#endif /* CUSTOM_INC_CONFIG_H_ */
