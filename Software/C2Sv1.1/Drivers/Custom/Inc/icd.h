/*
 * icd.h
 *
 * FISH <-> C2S interface control: frame layout and field values.
 * Values marked PLACEHOLDER are not yet agreed with Thomas; everything else
 * comes from the FISH request/response sheet. Code uses only these names,
 * so final values are a change to this file alone.
 *
 * Frame (max 256 bytes):
 *
 *   0      1       2        3       4          5     6 ...        last
 *   Start  Pkt-Id  Length   Source  Recipient  Type  Data (0-249) Checksum
 *   \_______ header ______/ \______ counted by Length _________/
 *
 * Length counts Source through Data, so Length = 3 + data bytes (max 252).
 * The REQUEST example on the sheet confirms it: Length = 0x03 with only
 * Source, Recipient and Type following.
 *
 * ACK/NAK frames on the whiteboard carry Length = 0 and nothing else
 * (Start, Pkt-Id, Length, Checksum). The parser accepts both forms; whether
 * ACK/NAK also carry Source/Recipient/Type is still to be confirmed.
 *
 * Plain C, no HAL: also compiled by the host tests in Testing/C2Sv1.1/protocol.
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_ICD_H_
#define CUSTOM_INC_ICD_H_

#define ICD_START_BYTE			0xFFu

// Packet-Id
#define ICD_ID_REQUEST			0xF0u
#define ICD_ID_RESPONSE			0xF1u
#define ICD_ID_ACK				0xF2u	// PLACEHOLDER
#define ICD_ID_NAK				0xF3u	// PLACEHOLDER

// Source / Recipient
#define ICD_NODE_FISH			0x01u
#define ICD_NODE_C2S			0x03u

// Type
#define ICD_TYPE_IMAGE			0x00u
#define ICD_TYPE_VOLTAGE		0x01u	// may be dropped: FISH may read the sensor itself

// Sizes
#define ICD_FRAME_MAX			256u
#define ICD_HEADER_BYTES		3u		// Start, Pkt-Id, Length
#define ICD_ADDR_BYTES			3u		// Source, Recipient, Type
#define ICD_CHECKSUM_BYTES		1u
#define ICD_DATA_MAX			(ICD_FRAME_MAX - ICD_HEADER_BYTES - ICD_ADDR_BYTES - ICD_CHECKSUM_BYTES)	// 249
#define ICD_LENGTH_MAX			(ICD_ADDR_BYTES + ICD_DATA_MAX)												// 252

// Checksum (U12): 8-bit sum of every byte before the checksum. PLACEHOLDER:
// the algorithm is not specified, and the sheet says the start byte "might
// change to" be excluded. Both live only in icd_checksum().
#define ICD_CHECKSUM_INCLUDES_START	1

#endif /* CUSTOM_INC_ICD_H_ */
