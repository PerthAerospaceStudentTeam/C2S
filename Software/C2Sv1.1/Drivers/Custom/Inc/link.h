/*
 * link.h
 *
 *  Created on: 22 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_LINK_H_
#define CUSTOM_INC_LINK_H_

#include "main.h"

void link_init(void);

uint32_t link_receive(uint8_t *buf, uint32_t cap);

void link_send(const uint8_t *frame, uint32_t len);

#endif
