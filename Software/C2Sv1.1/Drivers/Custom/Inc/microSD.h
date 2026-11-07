/*
 * microSD.h
 *
 *  Created on: 28 Oct 2026
 *      Author: felix
 *
 */

#ifndef CUSTOM_INC_MICROSD_H_
#define CUSTOM_INC_MICROSD_H_

#include "fatfs.h"
#include "stdio.h"
#include "ina219.h"
#include "main.h"
#include "ff.h"
#include "imaging.h"

// --- Housekeeping CSV log ---
HAL_StatusTypeDef microsd_create_indexed_file(char *filename);

HAL_StatusTypeDef microsd_init_file(char *filename);

HAL_StatusTypeDef microsd_append_file(char *filename, CSA_HandleTypeDef csa, uint8_t sys_status, imaging_req_t g_imaging_req);

HAL_StatusTypeDef microsd_mount();

// --- Image store ---
#define IMG_STORE_DEPTH		8	// Maximum number of images kept on the SD card

/** Return codes for the image store. */
typedef enum {
	STORE_OK = 0,
	STORE_ERR_NOT_READY,	/**< img_store_init has not succeeded */
	STORE_ERR_FS,			/**< FatFs error */
	STORE_ERR_NONE,			/**< No matching image */
} store_status_t;

/** A file being written to the store. */
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

#endif /* CUSTOM_INC_MICROSD_H_ */
