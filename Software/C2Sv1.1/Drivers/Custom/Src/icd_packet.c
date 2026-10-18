/*
 * icd_packet.c
 *
 * Build and parse FISH <-> C2S frames. See icd_packet.h and icd.h.
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#include "icd_packet.h"
#include <string.h>

uint8_t icd_checksum(const uint8_t *frame, uint32_t len)
{
	uint8_t sum = 0;

	for (uint32_t i = ICD_CHECKSUM_INCLUDES_START ? 0u : 1u; i < len; i++) {
		sum = (uint8_t)(sum + frame[i]);
	}
	return sum;
}

/**
 * @brief  Length is 0 (bare frame) or covers at least Source/Recipient/Type
 *         and at most a full data field.
 */
static int length_valid(uint8_t length)
{
	return length == 0u || (length >= ICD_ADDR_BYTES && length <= ICD_LENGTH_MAX);
}

uint32_t icd_build(const icd_packet_t *pkt, uint8_t *out, uint32_t out_cap)
{
	if (pkt == NULL || out == NULL) {
		return 0;
	}

	uint32_t length = 0;
	if (pkt->has_addr) {
		if (pkt->data_len > ICD_DATA_MAX) {
			return 0;
		}
		length = ICD_ADDR_BYTES + pkt->data_len;
	} else if (pkt->data_len != 0u) {
		return 0;	// a bare frame has nowhere to put data
	}

	uint32_t total = ICD_HEADER_BYTES + length + ICD_CHECKSUM_BYTES;
	if (out_cap < total) {
		return 0;
	}

	out[0] = ICD_START_BYTE;
	out[1] = pkt->id;
	out[2] = (uint8_t)length;
	if (pkt->has_addr) {
		out[3] = pkt->source;
		out[4] = pkt->recipient;
		out[5] = pkt->type;
		memcpy(&out[6], pkt->data, pkt->data_len);
	}
	out[total - 1u] = icd_checksum(out, total - 1u);
	return total;
}

void icd_parser_init(icd_parser_t *p)
{
	memset(p, 0, sizeof(*p));
}

/**
 * @brief  Remove the first `n` bytes of the buffer.
 */
static void consume(icd_parser_t *p, uint16_t n)
{
	memmove(p->buf, &p->buf[n], (size_t)(p->len - n));
	p->len = (uint16_t)(p->len - n);
}

/**
 * @brief  Discard bytes until the buffer starts with a start byte (or is empty).
 */
static void skip_to_start(icd_parser_t *p)
{
	uint16_t k = 0;
	while (k < p->len && p->buf[k] != ICD_START_BYTE) {
		k++;
	}
	p->discarded += k;
	consume(p, k);
}

/**
 * @brief  The candidate frame at buf[0] is bad. Drop its start byte and
 *         rescan from the next 0xFF in the bytes already received.
 */
static void resync(icd_parser_t *p)
{
	p->discarded++;
	consume(p, 1);
	skip_to_start(p);
}

static void decode(icd_parser_t *p)
{
	icd_packet_t *pkt = &p->pkt;
	uint8_t length = p->buf[2];

	pkt->id = p->buf[1];
	pkt->has_addr = (length != 0u);

	if (pkt->has_addr) {
		pkt->source    = p->buf[3];
		pkt->recipient = p->buf[4];
		pkt->type      = p->buf[5];
		pkt->data_len  = (uint8_t)(length - ICD_ADDR_BYTES);
		memcpy(pkt->data, &p->buf[6], pkt->data_len);
	} else {
		pkt->source = pkt->recipient = pkt->type = 0;
		pkt->data_len = 0;
	}
}

/**
 * @brief  Emit or reject every frame the buffer holds. Returns when the
 *         buffer is empty or holds the start of a frame still arriving.
 */
static void process(icd_parser_t *p, icd_frame_cb cb, void *ctx)
{
	for (;;) {
		skip_to_start(p);
		if (p->len < ICD_HEADER_BYTES) {
			return;
		}

		uint8_t length = p->buf[2];
		if (!length_valid(length)) {
			p->bad_length++;
			resync(p);
			continue;
		}

		uint16_t total = (uint16_t)(ICD_HEADER_BYTES + length + ICD_CHECKSUM_BYTES);
		if (p->len < total) {
			return;
		}

		if (icd_checksum(p->buf, total - 1u) != p->buf[total - 1u]) {
			p->bad_checksum++;
			resync(p);
			continue;
		}

		decode(p);
		consume(p, total);
		p->frames++;
		if (cb != NULL) {
			cb(&p->pkt, ctx);
		}
	}
}

void icd_parser_feed(icd_parser_t *p, const uint8_t *bytes, uint32_t n,
		icd_frame_cb cb, void *ctx)
{
	for (uint32_t i = 0; i < n; i++) {
		if (p->len == 0u && bytes[i] != ICD_START_BYTE) {
			p->discarded++;
			continue;
		}

		// process() always leaves len below the pending frame's total, and a
		// frame is at most ICD_FRAME_MAX bytes, so there is room for one more.
		p->buf[p->len++] = bytes[i];
		process(p, cb, ctx);
	}
}

void icd_parser_idle(icd_parser_t *p, icd_frame_cb cb, void *ctx)
{
	while (p->len > 0u) {
		resync(p);
		process(p, cb, ctx);
	}
}
