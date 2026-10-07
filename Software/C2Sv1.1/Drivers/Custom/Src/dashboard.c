/*
 * dashboard.c
 *
 *  Created on: 7 Oct 2026
 *      Author: felix
 */

#include "dashboard.h"


/**
  * @brief Sends data in packet format
  *
  * @param packet_id: Identifier of the data being sent (0x01: Image, 0x02: CSA)
  * @param data: Data to be sent in 8-bit pointer format
  * @param total_bytes: Total Bytes of image
  */

void Dashboard_Transmit(uint8_t packet_id, uint8_t * data, uint32_t total_bytes)
{
    uint16_t start_packet = 0xAA55;
    uint8_t end_packet = 0xBB;

    while (CDC_Transmit_HS((uint8_t*)(&start_packet), 2) == USBD_BUSY);
    while (CDC_Transmit_HS(&packet_id, 1) == USBD_BUSY);
    while (CDC_Transmit_HS((uint8_t*)(&total_bytes), 4) == USBD_BUSY);
    while (CDC_Transmit_HS(data, total_bytes) == USBD_BUSY);
    while (CDC_Transmit_HS((uint8_t*)(&end_packet), 1) == USBD_BUSY);
}
