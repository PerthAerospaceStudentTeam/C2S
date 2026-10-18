/*
 * img_store.c
 *
 * Rolling image store on the SD card. See img_store.h.
 *
 * The index (s_ids) lives in RAM and is rebuilt from the card at boot. It is
 * shared by the imaging task (begin/commit) and the comms task
 * (acquire/release), so it is guarded by s_lock. FatFs itself is reentrant
 * (_FS_REENTRANT), so file operations need no extra locking.
 *
 * Open files: imaging's .TMP plus comms' held .JPG = 2, which is the
 * current _FS_LOCK limit. Anything else that keeps a file open at the same
 * time (e.g. the benchmark harness) needs _FS_LOCK raised in CubeMX.
 *
 *  Created on: 11 Oct 2026
 *      Author: gabe
 */

#include "img_store.h"
#include "fatfs.h"
#include "cmsis_os2.h"
#include <string.h>

#define STORE_DIR		"IMG"
#define ID_DIGITS		7			// IMG/0000041.JPG: 10 million ids before wrapping
#define PATH_LEN		20

_Static_assert(IMG_STORE_DEPTH >= 2, "eviction needs one image besides the held one");

typedef enum { NAME_OTHER, NAME_JPG, NAME_TMP } name_kind_t;

static osMutexId_t s_lock;
static uint8_t  s_ready;
static uint32_t s_ids[IMG_STORE_DEPTH + 1];		// ascending; one spare during commit
static uint32_t s_count;
static uint32_t s_next_id;

// The image comms is serving. Only the comms task changes it.
static struct {
	uint8_t  active;
	uint32_t id;
	uint32_t size;
	FIL      file;
} s_hold;

// Directory scan objects. FILINFO carries a 256-byte long name, so keep it
// off the task stack.
static DIR s_dir;
static FILINFO s_info;

/**
 * @brief  Build "IMG/0000041.JPG" (or .TMP) without printf.
 */
static void make_path(char *out, uint32_t id, const char *ext)
{
	memcpy(out, STORE_DIR "/", 4);
	for (int i = ID_DIGITS - 1; i >= 0; i--) {
		out[4 + i] = (char)('0' + (id % 10u));
		id /= 10u;
	}
	out[4 + ID_DIGITS] = '.';
	memcpy(&out[5 + ID_DIGITS], ext, 4);	// 3 letters + NUL
}

/**
 * @brief  Recognise "0000041.JPG" / "0000041.TMP". Anything else on the card
 *         is someone else's file and is left alone.
 */
static name_kind_t parse_name(const char *name, uint32_t *id)
{
	uint32_t value = 0;

	for (int i = 0; i < ID_DIGITS; i++) {
		if (name[i] < '0' || name[i] > '9') {
			return NAME_OTHER;
		}
		value = value * 10u + (uint32_t)(name[i] - '0');
	}
	if (name[ID_DIGITS] != '.') {
		return NAME_OTHER;
	}

	*id = value;
	if (strcmp(&name[ID_DIGITS + 1], "JPG") == 0) return NAME_JPG;
	if (strcmp(&name[ID_DIGITS + 1], "TMP") == 0) return NAME_TMP;
	return NAME_OTHER;
}

/**
 * @brief  During the boot scan: add `id` to the index, keeping only the
 *         newest IMG_STORE_DEPTH ids, in ascending order.
 */
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

static uint8_t is_indexed(uint32_t id)
{
	for (uint32_t i = 0; i < s_count; i++) {
		if (s_ids[i] == id) {
			return 1;
		}
	}
	return 0;
}

/**
 * @brief  Find one file that should not be on the card: a leftover .TMP, or
 *         a .JPG that did not make the index. Deleting while a directory is
 *         being read is unsafe, so the caller deletes it and calls again.
 *
 * @retval 1 found (path filled in), 0 nothing left, -1 FatFs error
 */
static int find_stray_file(char *path)
{
	int found = 0;

	if (f_opendir(&s_dir, STORE_DIR) != FR_OK) {
		return -1;
	}

	for (;;) {
		if (f_readdir(&s_dir, &s_info) != FR_OK) {
			found = -1;
			break;
		}
		if (s_info.fname[0] == '\0') {
			break;
		}

		uint32_t id;
		name_kind_t kind = parse_name(s_info.fname, &id);
		if (kind == NAME_TMP || (kind == NAME_JPG && !is_indexed(id))) {
			make_path(path, id, (kind == NAME_TMP) ? "TMP" : "JPG");
			found = 1;
			break;
		}
	}

	f_closedir(&s_dir);
	return found;
}

/**
 * @brief  Drop the oldest image that is not being served. Lock held.
 * @retval 1 if one was removed from the index
 */
static int evict_oldest(void)
{
	uint32_t i = 0;
	while (i < s_count && s_hold.active && s_hold.id == s_ids[i]) {
		i++;
	}
	if (i == s_count) {
		return 0;
	}

	char path[PATH_LEN];
	make_path(path, s_ids[i], "JPG");
	f_unlink(path);		// if this fails, the next boot's cleanup removes it

	memmove(&s_ids[i], &s_ids[i + 1u], (s_count - i - 1u) * sizeof(s_ids[0]));
	s_count--;
	return 1;
}

store_status_t img_store_init(void)
{
	if (s_ready) {
		return STORE_OK;
	}

	if (s_lock == NULL) {
		s_lock = osMutexNew(NULL);
		if (s_lock == NULL) {
			return STORE_ERR_NOT_READY;
		}
	}

	if (f_mount(&SDFatFS, (TCHAR const *)SDPath, 1) != FR_OK) {
		return STORE_ERR_FS;
	}

	FRESULT fr = f_mkdir(STORE_DIR);
	if (fr != FR_OK && fr != FR_EXIST) {
		return STORE_ERR_FS;
	}

	// Pass 1: index the newest images already on the card.
	s_count = 0;
	if (f_opendir(&s_dir, STORE_DIR) != FR_OK) {
		return STORE_ERR_FS;
	}
	for (;;) {
		if (f_readdir(&s_dir, &s_info) != FR_OK) {
			f_closedir(&s_dir);
			return STORE_ERR_FS;
		}
		if (s_info.fname[0] == '\0') {
			break;
		}
		uint32_t id;
		if (parse_name(s_info.fname, &id) == NAME_JPG) {
			keep_newest(id);
		}
	}
	f_closedir(&s_dir);

	// Pass 2: delete unfinished .TMPs and images beyond the depth.
	char path[PATH_LEN];
	int found;
	while ((found = find_stray_file(path)) == 1) {
		if (f_unlink(path) != FR_OK) {
			return STORE_ERR_FS;
		}
	}
	if (found < 0) {
		return STORE_ERR_FS;
	}

	// Carry on numbering after the newest image that survived.
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
	if (!s_ready) return STORE_ERR_NOT_READY;
	if (slot == NULL) return STORE_ERR_PARAM;

	osMutexAcquire(s_lock, osWaitForever);
	slot->id = s_next_id++;
	osMutexRelease(s_lock);

	char path[PATH_LEN];
	make_path(path, slot->id, "TMP");

	slot->open = 0;
	if (f_open(&slot->file, path, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
		return STORE_ERR_FS;
	}
	slot->open = 1;
	return STORE_OK;
}

void img_store_abort(img_store_slot_t *slot)
{
	if (slot == NULL || !slot->open) {
		return;
	}

	f_close(&slot->file);
	slot->open = 0;

	char path[PATH_LEN];
	make_path(path, slot->id, "TMP");
	f_unlink(path);
}

store_status_t img_store_commit(img_store_slot_t *slot)
{
	if (!s_ready) return STORE_ERR_NOT_READY;
	if (slot == NULL || !slot->open) return STORE_ERR_PARAM;

	char tmp[PATH_LEN], jpg[PATH_LEN];
	make_path(tmp, slot->id, "TMP");
	make_path(jpg, slot->id, "JPG");

	FRESULT fr = f_close(&slot->file);
	slot->open = 0;
	if (fr == FR_OK) {
		fr = f_rename(tmp, jpg);
	}
	if (fr != FR_OK) {
		f_unlink(tmp);
		return STORE_ERR_FS;
	}

	osMutexAcquire(s_lock, osWaitForever);

	// Ids are handed out in increasing order and only the imaging task
	// commits, so appending keeps the index sorted.
	s_ids[s_count++] = slot->id;
	while (s_count > IMG_STORE_DEPTH && evict_oldest()) {
	}

	osMutexRelease(s_lock);
	return STORE_OK;
}

store_status_t img_store_acquire(uint32_t after_id, uint32_t *id, uint32_t *size)
{
	if (!s_ready) return STORE_ERR_NOT_READY;
	if (id == NULL || size == NULL) return STORE_ERR_PARAM;
	if (s_hold.active) return STORE_ERR_BUSY;

	osMutexAcquire(s_lock, osWaitForever);

	uint32_t i = 0;
	if (after_id != IMG_STORE_FROM_START) {
		while (i < s_count && s_ids[i] <= after_id) {
			i++;
		}
	}

	store_status_t st = STORE_ERR_NONE;
	if (i < s_count) {
		char path[PATH_LEN];
		make_path(path, s_ids[i], "JPG");

		if (f_open(&s_hold.file, path, FA_READ) != FR_OK) {
			st = STORE_ERR_FS;
		} else {
			s_hold.id = s_ids[i];
			s_hold.size = (uint32_t)f_size(&s_hold.file);
			s_hold.active = 1;
			*id = s_hold.id;
			*size = s_hold.size;
			st = STORE_OK;
		}
	}

	osMutexRelease(s_lock);
	return st;
}

store_status_t img_store_read(uint32_t id, uint32_t offset, void *buf, uint32_t len, uint32_t *got)
{
	if (got == NULL || (buf == NULL && len > 0u)) return STORE_ERR_PARAM;
	*got = 0;

	if (!s_hold.active || s_hold.id != id) {
		return STORE_ERR_NONE;
	}
	if (offset >= s_hold.size) {
		return STORE_OK;
	}
	if (len > s_hold.size - offset) {
		len = s_hold.size - offset;
	}

	UINT read = 0;
	if (f_lseek(&s_hold.file, offset) != FR_OK ||
		f_read(&s_hold.file, buf, len, &read) != FR_OK) {
		return STORE_ERR_FS;
	}

	*got = read;
	return STORE_OK;
}

void img_store_release(uint32_t id)
{
	if (!s_ready) {
		return;
	}

	osMutexAcquire(s_lock, osWaitForever);
	if (s_hold.active && s_hold.id == id) {
		f_close(&s_hold.file);
		s_hold.active = 0;
	}
	osMutexRelease(s_lock);
}
