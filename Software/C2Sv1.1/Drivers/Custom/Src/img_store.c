/*
 * img_store.c
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#include "img_store.h"
#include "fatfs.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STORE_DIR	"IMG"
#define TMP_PATH	STORE_DIR "/CURRENT.TMP"
#define PATH_LEN	20

static osMutexId_t s_lock;
static uint8_t  s_ready;
static uint32_t s_ids[IMG_STORE_DEPTH + 1];
static uint32_t s_count;
static uint32_t s_next_id;

static uint8_t  s_held;
static uint32_t s_held_id;
static FIL      s_held_file;

static DIR     s_dir;
static FILINFO s_info;

static void jpg_path(char *out, uint32_t id)
{
	snprintf(out, PATH_LEN, STORE_DIR "/%07lu.JPG", (unsigned long)id);
}

static int parse_jpg(const char *name, uint32_t *id)
{
	char *end;

	if (strlen(name) != 11 || strcmp(&name[7], ".JPG") != 0) {
		return 0;
	}
	*id = (uint32_t)strtoul(name, &end, 10);
	return end == &name[7];
}

static void keep_newest(uint32_t id)
{
	if (s_count == IMG_STORE_DEPTH) {
		if (id < s_ids[0]) {
			return;
		}
		memmove(&s_ids[0], &s_ids[1], (s_count - 1u) * sizeof(s_ids[0]));
		s_count--;
	}

	uint32_t i = s_count;
	while (i > 0u && s_ids[i - 1u] > id) {
		s_ids[i] = s_ids[i - 1u];
		i--;
	}
	s_ids[i] = id;
	s_count++;
}

static void evict_oldest(void)
{
	uint32_t i = (s_held && s_ids[0] == s_held_id) ? 1u : 0u;

	char path[PATH_LEN];
	jpg_path(path, s_ids[i]);
	f_unlink(path);

	memmove(&s_ids[i], &s_ids[i + 1u], (s_count - i - 1u) * sizeof(s_ids[0]));
	s_count--;
}

store_status_t img_store_init(void)
{
	s_lock = osMutexNew(NULL);

	if (f_mount(&SDFatFS, (TCHAR const *)SDPath, 1) != FR_OK) {
		return STORE_ERR_FS;
	}

	FRESULT fr = f_mkdir(STORE_DIR);
	if (fr != FR_OK && fr != FR_EXIST) {
		return STORE_ERR_FS;
	}

	if (f_opendir(&s_dir, STORE_DIR) != FR_OK) {
		return STORE_ERR_FS;
	}
	while (f_readdir(&s_dir, &s_info) == FR_OK && s_info.fname[0] != '\0') {
		uint32_t id;
		if (parse_jpg(s_info.fname, &id)) {
			keep_newest(id);
		}
	}
	f_closedir(&s_dir);

	s_next_id = (s_count > 0u) ? s_ids[s_count - 1u] + 1u : 0u;
	s_ready = 1;
	return STORE_OK;
}

uint8_t img_store_ready(void)
{
	return s_ready;
}

uint32_t img_store_count(void)
{
	return s_count;
}

store_status_t img_store_begin(img_store_slot_t *slot)
{
	if (!s_ready) {
		return STORE_ERR_NOT_READY;
	}

	slot->id = s_next_id++;
	if (f_open(&slot->file, TMP_PATH, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
		return STORE_ERR_FS;
	}
	return STORE_OK;
}

void img_store_abort(img_store_slot_t *slot)
{
	f_close(&slot->file);
	f_unlink(TMP_PATH);
}

store_status_t img_store_commit(img_store_slot_t *slot)
{
	char path[PATH_LEN];
	jpg_path(path, slot->id);

	if (f_close(&slot->file) != FR_OK || f_rename(TMP_PATH, path) != FR_OK) {
		f_unlink(TMP_PATH);
		return STORE_ERR_FS;
	}

	osMutexAcquire(s_lock, osWaitForever);
	s_ids[s_count++] = slot->id;
	if (s_count > IMG_STORE_DEPTH) {
		evict_oldest();
	}
	osMutexRelease(s_lock);
	return STORE_OK;
}

store_status_t img_store_acquire(uint32_t from_id, uint32_t *id)
{
	if (!s_ready) {
		return STORE_ERR_NOT_READY;
	}

	store_status_t st = STORE_ERR_NONE;
	osMutexAcquire(s_lock, osWaitForever);

	for (uint32_t i = 0; i < s_count; i++) {
		if (s_ids[i] >= from_id) {
			char path[PATH_LEN];
			jpg_path(path, s_ids[i]);

			st = STORE_ERR_FS;
			if (f_open(&s_held_file, path, FA_READ) == FR_OK) {
				s_held = 1;
				s_held_id = s_ids[i];
				*id = s_held_id;
				st = STORE_OK;
			}
			break;
		}
	}

	osMutexRelease(s_lock);
	return st;
}

store_status_t img_store_read(uint32_t offset, void *buf, uint32_t len, uint32_t *got)
{
	UINT n = 0;

	FRESULT fr = f_lseek(&s_held_file, offset);
	if (fr == FR_OK) {
		fr = f_read(&s_held_file, buf, len, &n);
	}

	*got = n;
	return (fr == FR_OK) ? STORE_OK : STORE_ERR_FS;
}

void img_store_release(void)
{
	osMutexAcquire(s_lock, osWaitForever);
	f_close(&s_held_file);
	s_held = 0;
	osMutexRelease(s_lock);
}
