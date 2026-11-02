/*
 * microSD.h
 *
 *  Created on: 28 Oct 2026
 *      Author: felix
 */

#ifndef CUSTOM_INC_MICROSD_H_
#define CUSTOM_INC_MICROSD_H_

#include "fatfs.h"
#include "stdio.h"
#include "ina219.h"

HAL_StatusTypeDef microsd_create_indexed_file(char *filename);

HAL_StatusTypeDef microsd_init_file(char *filename);

HAL_StatusTypeDef microsd_append_file(char *filename, CSA_HandleTypeDef csa, uint8_t cam_status);

HAL_StatusTypeDef microsd_mount();

#endif /* CUSTOM_INC_MICROSD_H_ */
