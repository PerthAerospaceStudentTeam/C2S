/*
 * comms.c
 *
 *  Created on: 18 Oct 2026
 *      Author: gabe
 */

#include "comms.h"
#include "link.h"
#include "icd.h"
#include "microSD.h"
#include "cmsis_os2.h"
#include <string.h>
#include "imaging.h"

volatile comms_stats_t g_comms_stats;

static uint8_t      s_rx[ICD_FRAME_MAX];
static uint8_t      s_frame[ICD_FRAME_MAX];
static icd_packet_t s_req;
static icd_packet_t s_res;

static uint32_t s_id;			// ID of the image being sent
static uint32_t s_offset;		// Byte offset of the next chunk in the image
static uint32_t s_chunk_len;	// Length of the chunk awaiting an ACK
static uint32_t s_next_id;		// Lowest image ID to serve next

extern volatile imaging_req_t g_imaging_req;

/**
 * @brief  Builds a response frame from s_res and sends it over the link.
 *
 * @param type ICD packet type.
 * @param len Number of valid bytes in s_res.data.
 */
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

/**
 * @brief  Advances the image offset past the chunk that was just acknowledged.
 */
static void on_ack(void)
{
	s_offset += s_chunk_len;
	s_chunk_len = 0;
}

/**
 * @brief  Sends a heartbeat: uptime in seconds (4 bytes LE), store ready flag, stored image count.
 */
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

/**
 * @brief  Add FISH request to imaging queue
 */
static void req_image(void)
{
	if (g_imaging_req.src == IMG_SRC_C2S) g_imaging_req.buf = 0; // Housekeeping requests are lowest priority, remove from queue
	g_imaging_req.buf = g_imaging_req.buf + 1;
	g_imaging_req.src = IMG_SRC_FISH;
}

/**
 * @brief  Dispatches a parsed packet. Packets addressed to other nodes are ignored.
 *
 * @param *pkt Pointer to the received packet.
 */
static void handle(const icd_packet_t *pkt)
{
	if (pkt->has_addr && pkt->recipient != ICD_NODE_C2S) {
		return;
	}

	g_comms_stats.frames_rx++;

	if (pkt->id == ICD_ID_REQUEST && pkt->type == ICD_TYPE_IMAGE) {

		req_image();

	} else if (pkt->id == ICD_ID_REQUEST && pkt->type == ICD_TYPE_HEARTBEAT) {

		send_heartbeat();

	} else if (pkt->id == ICD_ID_ACK) {

		on_ack();

	}
}

/**
 * @brief  Comms task body: receives frames and handles requests.
 *
 * Never returns.
 */
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
