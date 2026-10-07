/*
 * dashboard.h
 *
 *  Created on: 7 Oct 2026
 *      Author: felix
 */

#ifndef CUSTOM_INC_DASHBOARD_H_
#define CUSTOM_INC_DASHBOARD_H_

#include "main.h"

#include "usbd_cdc_if.h"

void Dashboard_Transmit(uint8_t packet_id, uint8_t * data, uint32_t total_bytes);

#endif /* CUSTOM_INC_DASHBOARD_H_ */
