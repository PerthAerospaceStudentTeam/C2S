/*
 * microSD.c
 *
 *  Created on: 28 Oct 2026
 *      Author: felix
 *
 */

#include "microSD.h"
#include "cmsis_os2.h"
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "imaging.h"

#define FILE_INDEX_INT_LIMIT 9999

#define STORE_DIR	"IMG"
#define TMP_PATH	STORE_DIR "/CURRENT.TMP"
#define PATH_LEN	20

extern FATFS SDFatFS;

// --- Housekeeping CSV log ---
static FRESULT fr;
static FIL file;
static FILINFO fno;

// --- Image store ---
static osMutexId_t s_lock;
static uint8_t  s_ready;
static uint32_t s_ids[IMG_STORE_DEPTH + 1];	// Stored image IDs, sorted oldest to newest
static uint32_t s_count;
static uint32_t s_next_id;

static uint8_t  s_held;			// 1 while an image is open for reading
static uint32_t s_held_id;
static FIL      s_held_file;

static DIR     s_dir;
static FILINFO s_info;

/* ======================================================================
 *  Housekeeping CSV log
 * ====================================================================== */

/**
 * @brief  Creates the next unused "c2s_data_index_N.csv" file.
 *
 * @param filename Buffer that receives the file name (at least 24 bytes).
 *
 * @retval HAL_OK on success, HAL_ERROR if all indexes are used or creating the file failed.
 */
HAL_StatusTypeDef microsd_create_indexed_file(char *filename)
{
	int lo = 0;                     // first unused index is >= lo
	int hi = FILE_INDEX_INT_LIMIT;  // first unused index is <= hi
	FRESULT r;

	// If the last index already exists, every slot is used
	sprintf(filename, "c2s_data_index_%d.csv", hi);
	r = f_stat(filename, &fno);
	if (r != FR_NO_FILE) return HAL_ERROR;   // exists (full) or a real SD error

	// Binary search for the first unused index
	while (lo < hi) {
		int mid = lo + (hi - lo) / 2;
		sprintf(filename, "c2s_data_index_%d.csv", mid);
		r = f_stat(filename, &fno);

		if (r == FR_OK) lo = mid + 1;		// exists, so look higher
		else if (r == FR_NO_FILE) hi = mid;	// free, so look at or below
		else return HAL_ERROR;
	}

	// lo == hi == first unused index
	sprintf(filename, "c2s_data_index_%d.csv", lo);
	fr = f_open(&file, filename, FA_WRITE | FA_CREATE_NEW);
	if (fr != FR_OK) return HAL_ERROR;

	f_close(&file);
	return HAL_OK;
}

/**
 * @brief  Writes the CSV header row to the data file.
 *
 * Any existing file with the same name is overwritten.
 *
 * @param filename Name of the file to open (created if it doesn't exist).
 *
 * @retval HAL_OK on success, HAL_ERROR on any SD card failure.
 */
HAL_StatusTypeDef microsd_init_file(char *filename)
{

	// Open the file, creating it or overwriting it
	fr = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
	if (fr != FR_OK) return HAL_ERROR;

	// Write the CSV header
	int n = f_printf(&file, "osTick,BusVoltage(mV),PowerDraw(mW),sysStatus,prevImgIdx,prevImgSrc,prevImgOsTick\n");

	FRESULT r = f_close(&file);

	return (n > 0 && r == FR_OK) ? HAL_OK : HAL_ERROR;
}

/**
 * @brief  Appends one row of housekeeping readings to the data file.
 *
 * Row format: tick (ms), bus voltage (mV), power (mW), current (mA), camera status.
 *
 * @param filename 		Name of the file to append to (created if it doesn't exist).
 * @param csa      		INA219 handle holding the latest readings.
 * @param sys_status   	System status flag written to the sysStatus column.
 *
 * @retval HAL_OK on success, HAL_ERROR on any SD card failure.
 */
HAL_StatusTypeDef microsd_append_file(char *filename, CSA_HandleTypeDef csa, uint8_t sys_status, imaging_req_t req)
{
	FRESULT r = f_open(&file, filename, FA_WRITE | FA_OPEN_APPEND);
	if (r != FR_OK) return HAL_ERROR;

	int n = f_printf(&file, "%lu,%u,%u,%u,%lu,%u,%lu\n",
	                 (unsigned long)xTaskGetTickCount(),
	                 (unsigned)(csa.bus_voltage / 4),
	                 (unsigned)csa.power_mW,
	                 (unsigned)sys_status,
					 (unsigned)req.idx,
					 (unsigned)req.src,
					 (unsigned long)req.timestamp);

	r = f_close(&file);                       // always close; this flushes the data
	return (n > 0 && r == FR_OK) ? HAL_OK : HAL_ERROR;
}

/**
 * @brief  Mounts the SD card.
 *
 * @retval HAL_OK on success, HAL_ERROR if the mount failed.
 */
HAL_StatusTypeDef microsd_mount()
{
    return (f_mount(&SDFatFS, (TCHAR const *)SDPath, 1) == FR_OK) ? HAL_OK : HAL_ERROR;
}

/* ======================================================================
 *  Image store
 * ====================================================================== */

/**
 * @brief  Builds the file path for an image ID, e.g. IMG/0000042.JPG (extension is IMG_FILE_EXT)
 *
 * @param *out Buffer of at least PATH_LEN bytes.
 * @param id Image ID.
 */
static void img_path(char *out, uint32_t id)
{
	snprintf(out, PATH_LEN, STORE_DIR "/%07lu" IMG_FILE_EXT, (unsigned long)id);
}

/**
 * @brief  Parses an image ID from a file name of the form NNNNNNN.JPG (extension is IMG_FILE_EXT)
 *
 * @param *name File name to parse.
 * @param *id Pointer to the parsed ID.
 *
 * @retval 1 if the name matched, 0 otherwise
 */
static int parse_img(const char *name, uint32_t *id)
{
	char *end;

	if (strlen(name) != 11 || strcmp(&name[7], IMG_FILE_EXT) != 0) {
		return 0;
	}
	*id = (uint32_t)strtoul(name, &end, 10);
	return end == &name[7];
}

/**
 * @brief  Inserts an ID into the sorted list during the startup scan, dropping the oldest when full.
 *
 * @param id Image ID found on the card.
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

/**
 * @brief  Deletes the oldest image from the card and the list.
 *
 * Skips the oldest image if it is currently held open for reading.
 */
static void evict_oldest(void)
{
	uint32_t i = (s_held && s_ids[0] == s_held_id) ? 1u : 0u;

	char path[PATH_LEN];
	img_path(path, s_ids[i]);
	f_unlink(path);

	memmove(&s_ids[i], &s_ids[i + 1u], (s_count - i - 1u) * sizeof(s_ids[0]));
	s_count--;
}

/**
 * @brief  Creates the image directory and indexes existing images.
 *
 * @retval STORE_OK on success, STORE_ERR_FS otherwise
 */
volatile FRESULT g_store_fr;
store_status_t img_store_init(void)
{
	if (s_lock == NULL) s_lock = osMutexNew(NULL);	// create once, not on every retry
	s_count = 0;									// rescan from scratch on retry

	g_store_fr = f_mkdir(STORE_DIR);
	if (g_store_fr != FR_OK && g_store_fr != FR_EXIST) return STORE_ERR_FS;

	g_store_fr = f_opendir(&s_dir, STORE_DIR);
	if (g_store_fr != FR_OK) return STORE_ERR_FS;

	while (f_readdir(&s_dir, &s_info) == FR_OK && s_info.fname[0] != '\0') {
		uint32_t id;
		if (parse_img(s_info.fname, &id)) {
			keep_newest(id);
		}
	}
	f_closedir(&s_dir);

	s_next_id = (s_count > 0u) ? s_ids[s_count - 1u] + 1u : 0u;
	s_ready = 1;
	return STORE_OK;
}

/**
 * @brief  Reports whether the store was initialised successfully.
 *
 * @retval 1 if ready, 0 otherwise
 */
uint8_t img_store_ready(void)
{
	return s_ready;
}

/**
 * @brief  Gets the number of images currently stored.
 *
 * @retval Image count
 */
uint32_t img_store_count(void)
{
	return s_count;
}

/**
 * @brief  Opens a temporary file for a new image and assigns it the next ID.
 *
 * @param *slot Pointer to the slot to fill. Write the image to slot->file.
 *
 * @retval STORE_OK on success, STORE_ERR_NOT_READY or STORE_ERR_FS otherwise
 */
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

/**
 * @brief  Discards a slot: closes and deletes the temporary file.
 *
 * @param *slot Pointer to the slot from img_store_begin.
 */
void img_store_abort(img_store_slot_t *slot)
{
	f_close(&slot->file);
	f_unlink(TMP_PATH);
}

/**
 * @brief  Finalises a slot: closes the file, renames it to its image path and adds it to the list.
 *
 * Evicts the oldest image if the store is over IMG_STORE_DEPTH.
 *
 * @param *slot Pointer to the slot from img_store_begin.
 *
 * @retval STORE_OK on success, STORE_ERR_FS otherwise
 */
store_status_t img_store_commit(img_store_slot_t *slot)
{
	char path[PATH_LEN];
	img_path(path, slot->id);

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

/**
 * @brief  Opens the oldest stored image with an ID of at least from_id for reading.
 *
 * Only one image can be held at a time. Call img_store_release when finished.
 *
 * @param from_id Lowest acceptable image ID.
 * @param *id Pointer to the ID of the opened image.
 *
 * @retval STORE_OK on success, STORE_ERR_NOT_READY, STORE_ERR_NONE (no match) or STORE_ERR_FS otherwise
 */
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
			img_path(path, s_ids[i]);

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

/**
 * @brief  Reads from the held image.
 *
 * @param offset Byte offset into the image.
 * @param *buf Buffer to read into.
 * @param len Maximum number of bytes to read.
 * @param *got Pointer to the number of bytes actually read (0 at end of file).
 *
 * @retval STORE_OK on success, STORE_ERR_FS otherwise
 */
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

/**
 * @brief  Closes the held image so it can be evicted.
 */
void img_store_release(void)
{
	osMutexAcquire(s_lock, osWaitForever);
	f_close(&s_held_file);
	s_held = 0;
	osMutexRelease(s_lock);
}
