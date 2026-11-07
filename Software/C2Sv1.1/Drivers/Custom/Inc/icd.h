/*
 * icd.h
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 *
 */

#ifndef CUSTOM_INC_ICD_H_
#define CUSTOM_INC_ICD_H_

#include <stdint.h>

// --- Frame constants ---
#define ICD_START_BYTE			0xFFu

#define ICD_ID_REQUEST			0xF0u
#define ICD_ID_RESPONSE			0xF1u
#define ICD_ID_ACK				0xF2u
#define ICD_ID_NAK				0xF3u

#define ICD_NODE_FISH			0x01u
#define ICD_NODE_C2S			0x03u

#define ICD_TYPE_IMAGE			0x00u
#define ICD_TYPE_HEARTBEAT		0x02u

#define ICD_FRAME_MAX			256u
#define ICD_HEADER_BYTES		3u
#define ICD_ADDR_BYTES			3u
#define ICD_CHECKSUM_BYTES		1u
#define ICD_DATA_MAX			(ICD_FRAME_MAX - ICD_HEADER_BYTES - ICD_ADDR_BYTES - ICD_CHECKSUM_BYTES)
#define ICD_LENGTH_MAX			(ICD_ADDR_BYTES + ICD_DATA_MAX)

/** Decoded ICD packet. Address fields are only valid when has_addr is set. */
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
