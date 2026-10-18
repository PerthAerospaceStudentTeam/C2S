/*
 * img_store.h
 *
 * Rolling store of compressed images on the SD card. Imaging adds images,
 * comms serves them to FISH; the store keeps the newest IMG_STORE_DEPTH and
 * deletes older ones, never the one currently being served.
 *
 * On the card:  IMG/0000041.TMP  while being encoded
 *               IMG/0000041.JPG  once complete (renamed only after f_close)
 * So a power cut mid-encode leaves a .TMP, which is deleted at the next boot,
 * never a broken .JPG.
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#ifndef CUSTOM_INC_IMG_STORE_H_
#define CUSTOM_INC_IMG_STORE_H_

#include "main.h"
#include "ff.h"

#define IMG_STORE_DEPTH		8		// U9: images kept; placeholder until decided

typedef enum {
	STORE_OK = 0,
	STORE_ERR_NOT_READY,	// img_store_init() has not succeeded
	STORE_ERR_FS,			// FatFs call failed
	STORE_ERR_NONE,			// no image matches the request
	STORE_ERR_BUSY,			// an image is already held; release it first
	STORE_ERR_PARAM,
} store_status_t;

// An image being written. Owned by the caller between begin and commit/abort.
typedef struct {
	uint32_t id;
	FIL      file;		// open for writing; pass to img_jpeg_encode()
	uint8_t  open;
} img_store_slot_t;

/**
 * @brief  Mount the card, create IMG/, delete leftover .TMP files and any
 *         images beyond the depth, and pick up numbering where it left off.
 *         Call once, from a task, before anything else in this module.
 */
store_status_t img_store_init(void);

uint8_t  img_store_ready(void);
uint32_t img_store_count(void);

// ---- Imaging side ----

/**
 * @brief  Allocate the next id and open IMG/<id>.TMP for writing.
 */
store_status_t img_store_begin(img_store_slot_t *slot);

/**
 * @brief  Close the file, rename it to .JPG, add it to the store and delete
 *         the oldest image(s) beyond IMG_STORE_DEPTH.
 *         On failure the slot is aborted (the .TMP is deleted).
 */
store_status_t img_store_commit(img_store_slot_t *slot);

/**
 * @brief  Close and delete an unfinished image.
 */
void img_store_abort(img_store_slot_t *slot);

// ---- Comms side (one held image at a time) ----

/**
 * @brief  Hold the oldest image with an id greater than `after_id` and open
 *         it for reading. A held image is never evicted. Pass
 *         IMG_STORE_FROM_START for the oldest image in the store.
 *         Returns STORE_ERR_NONE if there is nothing newer.
 */
#define IMG_STORE_FROM_START	0xFFFFFFFFu
store_status_t img_store_acquire(uint32_t after_id, uint32_t *id, uint32_t *size);

/**
 * @brief  Read up to `len` bytes at `offset` from the held image `id`.
 *         `*got` is 0 at or past the end of the image.
 */
store_status_t img_store_read(uint32_t id, uint32_t offset, void *buf, uint32_t len, uint32_t *got);

/**
 * @brief  Stop holding `id` so it can be evicted again.
 */
void img_store_release(uint32_t id);

#endif /* CUSTOM_INC_IMG_STORE_H_ */
