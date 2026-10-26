/*
 * icd_packet.h
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_ICD_PACKET_H_
#define CUSTOM_INC_ICD_PACKET_H_

#include <stdint.h>
#include "icd.h"

typedef struct {
	uint8_t id;
	uint8_t has_addr;
	uint8_t source;
	uint8_t recipient;
	uint8_t type;
	uint8_t data_len;
	uint8_t data[ICD_DATA_MAX];
} icd_packet_t;

uint8_t icd_checksum(const uint8_t *frame, uint32_t len);

uint32_t icd_build(const icd_packet_t *pkt, uint8_t out[ICD_FRAME_MAX]);

int icd_parse(const uint8_t *buf, uint32_t len, icd_packet_t *pkt);

#endif
