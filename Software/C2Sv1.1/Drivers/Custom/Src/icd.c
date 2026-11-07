/*
 * icd.c
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 *
 */

#include "icd.h"
#include <string.h>

/**
 * @brief  Calculates an 8-bit additive checksum.
 *
 * @param *frame Pointer to the bytes to sum.
 * @param len Number of bytes to sum.
 *
 * @retval Sum of all bytes, truncated to 8 bits
 */
uint8_t icd_checksum(const uint8_t *frame, uint32_t len)
{
	uint8_t sum = 0;

	for (uint32_t i = 0; i < len; i++) {
		sum = (uint8_t)(sum + frame[i]);
	}
	return sum;
}

/**
 * @brief  Serialises a packet into a frame: [start, id, length, (source, recipient, type, data...), checksum].
 *
 * @param *pkt Pointer to the packet to send.
 * @param out Buffer the frame is written into.
 *
 * @retval Total frame length in bytes, or 0 if data_len exceeds ICD_DATA_MAX
 */
uint32_t icd_build(const icd_packet_t *pkt, uint8_t out[ICD_FRAME_MAX])
{
	if (pkt->data_len > ICD_DATA_MAX) {
		return 0;
	}

	uint32_t length = pkt->has_addr ? ICD_ADDR_BYTES + pkt->data_len : 0u;

	out[0] = ICD_START_BYTE;
	out[1] = pkt->id;
	out[2] = (uint8_t)length;
	if (pkt->has_addr) {
		out[3] = pkt->source;
		out[4] = pkt->recipient;
		out[5] = pkt->type;
		memcpy(&out[6], pkt->data, pkt->data_len);
	}

	uint32_t end = ICD_HEADER_BYTES + length;
	out[end] = icd_checksum(out, end);
	return end + ICD_CHECKSUM_BYTES;
}

/**
 * @brief  Finds and decodes a frame in a received buffer.
 *
 * Skips any bytes before the start byte, then validates length and checksum.
 *
 * @param *buf Pointer to the received bytes.
 * @param len Number of received bytes.
 * @param *pkt Pointer to the packet filled on success.
 *
 * @retval 1 if a valid frame was decoded, 0 otherwise
 */
int icd_parse(const uint8_t *buf, uint32_t len, icd_packet_t *pkt)
{
	uint32_t start = 0;
	while (start < len && buf[start] != ICD_START_BYTE) {
		start++;
	}

	const uint8_t *f = &buf[start];
	uint32_t avail = len - start;
	if (avail < ICD_HEADER_BYTES + ICD_CHECKSUM_BYTES) {
		return 0;
	}

	uint8_t length = f[2];
	if (length != 0u && (length < ICD_ADDR_BYTES || length > ICD_LENGTH_MAX)) {
		return 0;
	}

	uint32_t end = ICD_HEADER_BYTES + length;
	if (avail < end + ICD_CHECKSUM_BYTES || icd_checksum(f, end) != f[end]) {
		return 0;
	}

	memset(pkt, 0, sizeof(*pkt));
	pkt->id = f[1];
	pkt->has_addr = (length != 0u);
	if (pkt->has_addr) {
		pkt->source    = f[3];
		pkt->recipient = f[4];
		pkt->type      = f[5];
		pkt->data_len  = (uint8_t)(length - ICD_ADDR_BYTES);
		memcpy(pkt->data, &f[6], pkt->data_len);
	}
	return 1;
}
