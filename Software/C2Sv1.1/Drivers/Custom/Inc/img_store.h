/*
 * img_store.h
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_IMG_STORE_H_
#define CUSTOM_INC_IMG_STORE_H_

#include "main.h"
#include "ff.h"

#define IMG_STORE_DEPTH		8

typedef enum {
	STORE_OK = 0,
	STORE_ERR_NOT_READY,
	STORE_ERR_FS,
	STORE_ERR_NONE,
} store_status_t;

typedef struct {
	uint32_t id;
	FIL      file;
} img_store_slot_t;

store_status_t img_store_init(void);
uint8_t  img_store_ready(void);
uint32_t img_store_count(void);

store_status_t img_store_begin(img_store_slot_t *slot);
store_status_t img_store_commit(img_store_slot_t *slot);
void img_store_abort(img_store_slot_t *slot);

store_status_t img_store_acquire(uint32_t from_id, uint32_t *id);
store_status_t img_store_read(uint32_t offset, void *buf, uint32_t len, uint32_t *got);
void img_store_release(void);

#endif
