/*
 * icd_packet.h
 *
 * Build and parse FISH <-> C2S frames (layout in icd.h).
 * Plain C, no HAL: also compiled by the host tests in Testing/C2Sv1.1/protocol.
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_ICD_PACKET_H_
#define CUSTOM_INC_ICD_PACKET_H_

#include <stdint.h>
#include "icd.h"

typedef struct {
	uint8_t id;					// ICD_ID_*
	uint8_t has_addr;			// 0: bare frame (Length = 0, e.g. ACK/NAK)
	uint8_t source;				// valid when has_addr
	uint8_t recipient;
	uint8_t type;
	uint8_t data_len;			// 0..ICD_DATA_MAX, only when has_addr
	uint8_t data[ICD_DATA_MAX];
} icd_packet_t;

/**
 * @brief  Checksum over `len` bytes of a frame, i.e. everything before the
 *         checksum byte. See ICD_CHECKSUM_INCLUDES_START.
 */
uint8_t icd_checksum(const uint8_t *frame, uint32_t len);

/**
 * @brief  Serialise `pkt` into `out`.
 * @retval frame length in bytes, or 0 if `pkt` is invalid or `out_cap` is too small
 */
uint32_t icd_build(const icd_packet_t *pkt, uint8_t *out, uint32_t out_cap);

// ---- Streaming parser ----

typedef void (*icd_frame_cb)(const icd_packet_t *pkt, void *ctx);

typedef struct {
	uint8_t  buf[ICD_FRAME_MAX];	// bytes from a candidate start byte onwards
	uint16_t len;
	icd_packet_t pkt;				// decode target handed to the callback

	// Counters for link diagnostics
	uint32_t frames;
	uint32_t bad_checksum;
	uint32_t bad_length;
	uint32_t discarded;				// bytes thrown away while resynchronising
} icd_parser_t;

void icd_parser_init(icd_parser_t *p);

/**
 * @brief  Feed received bytes. Calls `cb` once per valid frame, in order.
 *
 *         Frames are found by their 0xFF start byte. If a candidate frame
 *         turns out bad (impossible Length or wrong checksum), the parser
 *         drops only that start byte and rescans the bytes it already has
 *         from the next 0xFF. So a dropped or corrupted byte costs one
 *         frame, not the frame after it too.
 */
void icd_parser_feed(icd_parser_t *p, const uint8_t *bytes, uint32_t n,
		icd_frame_cb cb, void *ctx);

/**
 * @brief  Tell the parser the line has gone idle. FISH sends each frame in
 *         one burst, so anything still incomplete is broken: discard it and
 *         rescan, which may release a complete frame hidden behind it.
 *         Call on UART idle-line, or after an inter-byte timeout.
 */
void icd_parser_idle(icd_parser_t *p, icd_frame_cb cb, void *ctx);

#endif /* CUSTOM_INC_ICD_PACKET_H_ */
