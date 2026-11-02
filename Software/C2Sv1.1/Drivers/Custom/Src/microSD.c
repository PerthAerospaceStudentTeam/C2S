/*
 * microSD.c
 *
 *  Created on: 28 Oct 2026
 *      Author: felix
 */

#include "microSD.h"

extern FATFS SDFatFS;

FRESULT fr;
FIL file;
FILINFO fno;

/**
 * @brief  Mounts the SD card and creates the next unused "data_N.csv" file.
 *
 * @param filename Buffer that receives the file name (at least 20 bytes).
 *
 * @retval HAL_OK on success, HAL_ERROR if mounting or creating the file failed.
 */
HAL_StatusTypeDef microsd_create_indexed_file(char *filename)
{
	int file_index = 0;

	// Find the first index that doesn't exist yet
	do {
		sprintf(filename, "c2s_data_index_%d.csv", file_index);
		file_index++;
	} while (f_stat(filename, &fno) == FR_OK);

	// Create the file (header can be written here if needed)
	fr = f_open(&file, filename, FA_OPEN_ALWAYS);
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
	fr = f_open(&file, filename, FA_OPEN_ALWAYS | FA_OPEN_APPEND);
	if (fr != FR_OK) return HAL_ERROR;

	// Write the CSV header
	fr = f_printf(&file, "Tick,BusVoltage(mV),PowerDraw(mW),CurrentDraw(mA),CamActive\n");
	if (fr != FR_OK) return HAL_ERROR;

	f_close(&file);
	return HAL_OK;
}

/**
 * @brief  Appends one row of housekeeping readings to the data file.
 *
 * Row format: tick (ms), bus voltage (mV), power (mW), current (mA).
 *
 * @param filename Name of the file to append to (created if it doesn't exist).
 * @param ina      INA219 handle holding the latest readings.
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
