/*
 * microSD.c
 *
 *  Created on: 28 Oct 2026
 *      Author: felix
 */

#include "microSD.h"

#define FILE_INDEX_INT_LIMIT 9999

extern FATFS SDFatFS;

// Local PVs
static FRESULT fr;
static FIL file;
static FILINFO fno;

/**
 * @brief  Creates the next unused "data_N.csv" file.
 *
 * @param filename Buffer that receives the file name (at least 20 bytes).
 *
 * @retval HAL_OK on success, HAL_ERROR if mounting or creating the file failed.
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
 * @param filename Name of the file to open (created if it doesn't exist).
 *
 * @retval HAL_OK on success, HAL_ERROR on any SD card failure.
 */
HAL_StatusTypeDef microsd_init_file(char *filename)
{

	// Open the file for appending, creating it if needed
	fr = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
	if (fr != FR_OK) return HAL_ERROR;

	// Write the CSV header
	int n = f_printf(&file, "Tick,BusVoltage(mV),PowerDraw(mW),CurrentDraw(mA),CamActive\n");

	FRESULT r = f_close(&file);

	return (n > 0 && r == FR_OK) ? HAL_OK : HAL_ERROR;
}

/**
 * @brief  Appends one row of housekeeping readings to the data file.
 *
 * Row format: tick (ms), bus voltage (mV), power (mW), current (mA).
 *
 * @param filename Name of the file to append to (created if it doesn't exist).
 * @param csa      INA219 handle holding the latest readings.
 *
 * @retval HAL_OK on success, HAL_ERROR on any SD card failure.
 */
HAL_StatusTypeDef microsd_append_file(char *filename, CSA_HandleTypeDef csa, uint8_t status)
{
	FRESULT r = f_open(&file, filename, FA_WRITE | FA_OPEN_APPEND);
	if (r != FR_OK) return HAL_ERROR;

	int n = f_printf(&file, "%lu,%u,%u,%d,%u\n",
	                 (unsigned long)HAL_GetTick(),
	                 (unsigned)(csa.bus_voltage / 4),
	                 (unsigned)csa.power_mW,
	                 (int)(int16_t)csa.current_mA,
	                 (unsigned)status);

	r = f_close(&file);                       // always close; this flushes the data
	return (n > 0 && r == FR_OK) ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef microsd_mount()
{
    return (f_mount(&SDFatFS, (TCHAR const *)SDPath, 1) == FR_OK) ? HAL_OK : HAL_ERROR;
}
