/*
 * comms.h
 *
 *  Created on: 18 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_COMMS_H_
#define CUSTOM_INC_COMMS_H_

#include <stdint.h>

/** Running link counters. */
typedef struct {
	uint32_t frames_rx;		/**< Valid frames addressed to this node */
	uint32_t bad_frames;	/**< Frames with a start byte that failed to parse */
	uint32_t packets_sent;	/**< Image data chunks sent */
	uint32_t images_sent;	/**< Complete images sent */
} comms_stats_t;

extern volatile comms_stats_t g_comms_stats;

void comms_run(void);

#endif
