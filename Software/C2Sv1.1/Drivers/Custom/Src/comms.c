/*
 * comms.c
 *
 *  Created on: 18 Oct 2026
 *      Author: gabe
 */

#include "comms.h"
#include "link.h"
#include "icd_packet.h"
#include "img_store.h"
#include "cmsis_os2.h"
#include <string.h>

volatile comms_stats_t g_comms_stats;

static uint8_t      s_rx[ICD_FRAME_MAX];
static uint8_t      s_frame[ICD_FRAME_MAX];
static icd_packet_t s_req;
static icd_packet_t s_res;

static uint8_t  s_sending;
static uint32_t s_id;
static uint32_t s_offset;
static uint32_t s_chunk_len;
static uint32_t s_next_id;

static void send_response(uint8_t type, uint8_t len)
{
	s_res.id        = ICD_ID_RESPONSE;
	s_res.has_addr  = 1;
	s_res.source    = ICD_NODE_C2S;
	s_res.recipient = ICD_NODE_FISH;
	s_res.type      = type;
	s_res.data_len  = len;

	link_send(s_frame, icd_build(&s_res, s_frame));
}

static void serve_image(void)
{
	if (!s_sending) {
		if (img_store_acquire(s_next_id, &s_id) != STORE_OK) {
			send_response(ICD_TYPE_IMAGE, 0);
			return;
		}
		s_sending = 1;
		s_offset = 0;
		s_chunk_len = 0;
	}

	uint32_t got = 0;
	if (img_store_read(s_offset, s_res.data, ICD_DATA_MAX, &got) != STORE_OK) {
		return;
	}

	if (got == 0u) {
		send_response(ICD_TYPE_IMAGE, 0);
		img_store_release();
		s_sending = 0;
		s_next_id = s_id + 1u;
		g_comms_stats.images_sent++;
		return;
	}

	s_chunk_len = got;
	send_response(ICD_TYPE_IMAGE, (uint8_t)got);
	g_comms_stats.packets_sent++;
}

static void on_ack(void)
{
	s_offset += s_chunk_len;
	s_chunk_len = 0;
}

static void send_heartbeat(void)
{
	uint32_t uptime_s = osKernelGetTickCount() / 1000u;

	s_res.data[0] = (uint8_t)uptime_s;
	s_res.data[1] = (uint8_t)(uptime_s >> 8);
	s_res.data[2] = (uint8_t)(uptime_s >> 16);
	s_res.data[3] = (uint8_t)(uptime_s >> 24);
	s_res.data[4] = img_store_ready();
	s_res.data[5] = (uint8_t)img_store_count();
	send_response(ICD_TYPE_HEARTBEAT, 6);
}

static void handle(const icd_packet_t *pkt)
{
	if (pkt->has_addr && pkt->recipient != ICD_NODE_C2S) {
		return;
	}
	g_comms_stats.frames_rx++;

	if (pkt->id == ICD_ID_REQUEST && pkt->type == ICD_TYPE_IMAGE) {
		serve_image();
	} else if (pkt->id == ICD_ID_REQUEST && pkt->type == ICD_TYPE_HEARTBEAT) {
		send_heartbeat();
	} else if (pkt->id == ICD_ID_ACK) {
		on_ack();
	}
}

void comms_run(void)
{
	link_init();

	for (;;) {
		uint32_t n = link_receive(s_rx, sizeof(s_rx));

		if (icd_parse(s_rx, n, &s_req)) {
			handle(&s_req);
		} else if (memchr(s_rx, ICD_START_BYTE, n) != NULL) {
			g_comms_stats.bad_frames++;
		}
	}
}
