/*
 * icd.h
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_ICD_H_
#define CUSTOM_INC_ICD_H_

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

#endif
