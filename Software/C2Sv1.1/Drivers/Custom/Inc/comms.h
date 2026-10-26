/*
 * comms.h
 *
 *  Created on: 18 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_COMMS_H_
#define CUSTOM_INC_COMMS_H_

#include <stdint.h>

typedef struct {
	uint32_t frames_rx;
	uint32_t bad_frames;
	uint32_t packets_sent;
	uint32_t images_sent;
} comms_stats_t;

extern volatile comms_stats_t g_comms_stats;

void comms_run(void);

#endif
