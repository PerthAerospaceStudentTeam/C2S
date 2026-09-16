/*
 * AT25SF2561C.c
 *
 *  Created on: Feb 20, 2026
 *      Author: felix
 *  Finished on: Mar 6, 2026
 *
 *  Functions are named according to purpose [DRIVER]_[OEPRATION]_[OBJECT]
 *  	DRIVER: What the function is for, in this case, QSPI
 *  	OPERATION: Read, Write, Erase, Reset, etc.
 *  	OBJECT: What it is operating on e.g., WEL
 *
 *
 */


#include "AT25SF2561C.h"

/**
 * @brief  Resets the OSPI command structure to default values.
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param  QSPI_Memory->sCommand Pointer to the command structure.
 * @param *QSPI_Memory Memory struct for QSPI Memory
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 */
HAL_StatusTypeDef QSPI_Init_Memory(OSPI_HandleTypeDef *hospi, OSPI_RegularCmdTypeDef *sCommand, QSPI_HandleTypeDef *QSPI_Memory)
{
    // Fill out QSPI Struct
    QSPI_Memory->occupied_data = 0;
    QSPI_Memory->memory_size = (32 * 1024 * 1024);
	QSPI_Memory->hospi = hospi;
	QSPI_Memory->sCommand = sCommand;

    // Enable QE Bit (Critical for 4-line modes)
    if (QSPI_Write_StatusRegister(QSPI_Memory, 2, 1, 1) != HAL_OK) return HAL_ERROR;
    QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_GENERAL);

    // Enter 4-Byte Address Mode (B7h)
    if (QSPI_Command(QSPI_Memory, QSPI_ENTER_4BYTE_ADDRESS) != HAL_OK) return HAL_ERROR;
    QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_GENERAL); // Must wait here!

    // Define Dummy Cycles in SR3
	uint8_t dummy_bit_0;
	uint8_t dummy_bit_1;
	if (QSPI_DUMMY_CYCLES_NUM == 6) 		{ dummy_bit_1 = 0; dummy_bit_0 = 0; }
	else if (QSPI_DUMMY_CYCLES_NUM == 10) { dummy_bit_1 = 0; dummy_bit_0 = 1; }
	else if (QSPI_DUMMY_CYCLES_NUM == 14) { dummy_bit_1 = 1; dummy_bit_0 = 0; }
	else if (QSPI_DUMMY_CYCLES_NUM == 18) { dummy_bit_1 = 1; dummy_bit_0 = 1; }
	else return HAL_ERROR; // need custom message here
    QSPI_Write_StatusRegister(QSPI_Memory, 3, 3, dummy_bit_0);
    QSPI_Write_StatusRegister(QSPI_Memory, 3, 4, dummy_bit_1);

    // Read DEVICE ID (cannot be done in the first few lines since address and data modes needed to be set
    QSPI_Read_JedecId(QSPI_Memory);

    return QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_GENERAL);
}

/**
 * @brief  Assigns data to the QSPI DataBlock struct.
 *
 *
 * @param *DataBlock Pointer to datablock
 * @param  data_size Size of data
 * @param *tx_pData Pointer to data that will be written
 * @param *rx_pData Pointer to data that will be read into
 *
 */
void QSPI_Init_DataBlock(QSPI_DataChunk_HandleTypeDef *DataBlock, uint32_t data_size, void *tx_pData, void *rx_pData)
{
	DataBlock->data_size = data_size;
	DataBlock->tx_pData = tx_pData;
	DataBlock->rx_pData = rx_pData;
}

/**
 * @brief  Resets the OSPI command structure to default values.
 * @param  QSPI_Memory->sCommand Pointer to the command structure.
 */
void QSPI_Reset_Struct(QSPI_HandleTypeDef *QSPI_Memory)
{
    QSPI_Memory->sCommand->InstructionMode    = HAL_OSPI_INSTRUCTION_NONE;
    QSPI_Memory->sCommand->Instruction        = 0;
    QSPI_Memory->sCommand->AddressMode        = HAL_OSPI_ADDRESS_NONE;
    QSPI_Memory->sCommand->AddressSize        = HAL_OSPI_ADDRESS_32_BITS;
    QSPI_Memory->sCommand->Address            = 0;
    QSPI_Memory->sCommand->AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    QSPI_Memory->sCommand->DummyCycles        = 0;
    QSPI_Memory->sCommand->DataMode           = HAL_OSPI_DATA_NONE;
    QSPI_Memory->sCommand->NbData             = 0;
    QSPI_Memory->sCommand->DQSMode            = HAL_OSPI_DQS_DISABLE;
    QSPI_Memory->sCommand->SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
}

/**
 * @brief Retrieves the Status Register 1 Byte from the memory
 *
 * Corresponds to page 13 in the datasheet, Chapter 5.
 * Note that there is a typo in chapter 5.1 of the datasheet where the read command is meant to be 0x05 and not 0x01 (as outlined in 5)
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 * @param reg Either register (1,2,3)
 * @param *data Pointer to a byte to be read into
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 *
 */
HAL_StatusTypeDef QSPI_Read_StatusRegister(QSPI_HandleTypeDef *QSPI_Memory, uint8_t reg, uint8_t *data)
{
	// Allocate instruction based on register
	uint8_t cmd;
	if (reg == 1) cmd = QSPI_READ_SR1;
	else if (reg == 2) cmd = QSPI_READ_SR2;
	else if (reg == 3) cmd = QSPI_READ_SR3;
	else return HAL_ERROR;

	QSPI_Reset_Struct(QSPI_Memory);

	QSPI_Memory->sCommand->Instruction 		= cmd; // Read Status Register
	QSPI_Memory->sCommand->InstructionMode 	= HAL_OSPI_INSTRUCTION_1_LINE;
	QSPI_Memory->sCommand->AddressMode 		= HAL_OSPI_ADDRESS_NONE;
	QSPI_Memory->sCommand->DataMode 			= HAL_OSPI_DATA_1_LINE;
	QSPI_Memory->sCommand->NbData 			= 1;

	if (HAL_OSPI_Command(QSPI_Memory->hospi, QSPI_Memory->sCommand, QSPI_TIMEOUT_GENERAL) == HAL_OK) {
	    HAL_OSPI_Receive(QSPI_Memory->hospi, data, QSPI_TIMEOUT_GENERAL);
	    return HAL_OK;
	}
	else return HAL_ERROR;
}

/**
 * @brief Writes to a bit into one of the three status registers
 *
 * Corresponds to page 24 in the datasheet, Chapter 7 for the command. Page 13, chapter 5.1, for the status registers
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 * @param reg Either register (1,2,3)
 * @param bit Which bit of the register (0-7)
 * @param status Bit status (0 or 1)
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 *
 */
HAL_StatusTypeDef QSPI_Write_StatusRegister(QSPI_HandleTypeDef *QSPI_Memory, uint8_t reg, uint8_t bit, uint8_t status)
{
	if (QSPI_Command(QSPI_Memory, QSPI_WRITE_ENABLE) != HAL_OK) return HAL_ERROR;
    if (QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;

    // Validate Read-Only bits
    if ( (reg == 1) && (bit < 1) ) return HAL_ERROR; // WEL and RDY bits
    else if ( (reg == 2) && ((bit == 7) || bit == 2) ) return HAL_ERROR; // Suspend bits
    else if ( (reg == 3) && (bit == 0) ) return HAL_ERROR; // ADS bit

    uint8_t cmd;
    if (reg == 1)      cmd = QSPI_WRITE_SR1; // 01h
    else if (reg == 2) cmd = QSPI_WRITE_SR2; // 31h
    else if (reg == 3) cmd = QSPI_WRITE_SR3; // 11h
    else return HAL_ERROR;

    uint8_t current_val;
    if (QSPI_Read_StatusRegister(QSPI_Memory, reg, &current_val) != HAL_OK) return HAL_ERROR;

    // Clear the target bit, then OR it with the new status
    uint8_t new_val = current_val;
    if (status) {
        new_val |= (1 << bit);  // Set bit to 1
    } else {
        new_val &= ~(1 << bit); // Clear bit to 0
    }

    // If it's already in the desired state
    if (new_val == current_val) return HAL_OK;

    QSPI_Memory->sCommand->Instruction     = cmd;
    QSPI_Memory->sCommand->InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    QSPI_Memory->sCommand->AddressMode     = HAL_OSPI_ADDRESS_NONE;
    QSPI_Memory->sCommand->DataMode        = HAL_OSPI_DATA_1_LINE;
    QSPI_Memory->sCommand->NbData          = 1;

    if (HAL_OSPI_Command(QSPI_Memory->hospi, QSPI_Memory->sCommand, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;
    if (HAL_OSPI_Transmit(QSPI_Memory->hospi, &new_val, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;

    return HAL_OK;
}

/**
 * @brief Confirms whether it is okay to write or not to memory
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 *
 * @retval HAL_StatusTypeDef (HAL_OK if ready, HAL_TIMEOUT if not)
 *
 */
HAL_StatusTypeDef QSPI_Validate_Wel(QSPI_HandleTypeDef *QSPI_Memory, uint32_t Timeout)
{
    uint8_t status = 0;
    uint32_t tickstart = HAL_GetTick();

    while ((HAL_GetTick() - tickstart) < Timeout)
    {
        // Utilize your existing function
        QSPI_Read_StatusRegister(QSPI_Memory, 1, &status);

        // Check Bit 1 (WEL): 1 is Ready
        if ((status & 0x02) == 0x02)
        {
            return HAL_OK;
        }
    }
    return HAL_TIMEOUT;
}

/**
 * @brief  Blocks until the flash device is ready (RDY/BSY bit is 0)
 *
 * Must be called BEFORE the QSPI_Memory->sCommand struct is defined / at the start of a function
 *
 * @param  QSPI_Memory->hospi   OCTOSPI handle
 * @param  QSPI_Memory->sCommand OSPI command structure
 * @param  Timeout  Maximum wait time in ms
 * @retval HAL_StatusTypeDef (HAL_OK if ready, HAL_TIMEOUT if it stayed busy)
 */
HAL_StatusTypeDef QSPI_Validate_Rdy(QSPI_HandleTypeDef *QSPI_Memory, uint32_t Timeout)
{
    uint8_t status = 0;
    uint32_t tickstart = HAL_GetTick();

    while ((HAL_GetTick() - tickstart) < Timeout)
    {
        // Utilize your existing function
        QSPI_Read_StatusRegister(QSPI_Memory, 1, &status);

        // Check Bit 0 (RDY/BSY): 0 is Ready
        if ((status & 0x01) == 0)
        {
            return HAL_OK;
        }
    }
    return HAL_TIMEOUT;
}

/**
 * @brief Retrieves the JEDEC assigned Manufacturer ID and places into an array
 *
 * Corresponds to page 77 in the datasheet, Chapter 7.4.
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 * @param *ID_REG 3 Element array to store data
 *
 */
HAL_StatusTypeDef QSPI_Read_JedecId(QSPI_HandleTypeDef *QSPI_Memory)
{
	// High-level check before proceeding
	if (QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;

	// Command
	QSPI_Memory->sCommand->InstructionMode 		= HAL_OSPI_INSTRUCTION_1_LINE; // Can set to HAL_OSPI_INSTRUCTION_4_LINES
	QSPI_Memory->sCommand->Instruction 			= QSPI_READ_ID;
	QSPI_Memory->sCommand->AddressMode 			= HAL_OSPI_ADDRESS_NONE;
	QSPI_Memory->sCommand->AlternateBytesMode 	= HAL_OSPI_ALTERNATE_BYTES_NONE;
	QSPI_Memory->sCommand->DummyCycles 			= 0;
	QSPI_Memory->sCommand->Address 				= 0;
	QSPI_Memory->sCommand->DataDtrMode 			= HAL_OSPI_DATA_DTR_DISABLE;
	QSPI_Memory->sCommand->SIOOMode 				= HAL_OSPI_SIOO_INST_EVERY_CMD;
	QSPI_Memory->sCommand->NbData 				= 3;

	if (HAL_OSPI_Command(QSPI_Memory->hospi, QSPI_Memory->sCommand, QSPI_TIMEOUT_GENERAL) == HAL_OK) {
	    HAL_OSPI_Receive(QSPI_Memory->hospi, QSPI_Memory->ID_REG, QSPI_TIMEOUT_GENERAL);
	    return HAL_OK;
	}
	else return HAL_ERROR;
}

/**
 * @brief Sends the memory a command
 *
 * Corresponds to page 24 in the datasheet, Chapter 7
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 * @param cmd The command hex value
 *
 */
HAL_StatusTypeDef QSPI_Command(QSPI_HandleTypeDef *QSPI_Memory, uint8_t cmd)
{
	if (QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;

    QSPI_Memory->sCommand->Instruction     = cmd;
    QSPI_Memory->sCommand->InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    QSPI_Memory->sCommand->AddressMode     = HAL_OSPI_ADDRESS_NONE;
    QSPI_Memory->sCommand->DataMode        = HAL_OSPI_DATA_NONE;
    QSPI_Memory->sCommand->NbData          = 0;

    HAL_OSPI_Command(QSPI_Memory->hospi, QSPI_Memory->sCommand, QSPI_TIMEOUT_GENERAL);

    if (QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;

    return HAL_OK;
}

/**
 * @brief Puts the memory in or out of sleep
 *
 * Corresponds to page 24 in the datasheet, Chapter 7
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 * @param status Opcode specifying whether entering or exiting sleep
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 *
 */
HAL_StatusTypeDef QSPI_Command_Sleep(QSPI_HandleTypeDef *QSPI_Memory, uint8_t status)
{
    // Check whether already in desired state
	HAL_StatusTypeDef check = QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_GENERAL);
	// If it times out, the chip is likely already asleep or disconnected
	if ( (status == QSPI_ENTER_SLEEP) && (check == HAL_TIMEOUT) ) return HAL_OK;
	// If it returns OK, the chip is already awake and responsive
	if ( (status == QSPI_EXIT_SLEEP) && (check == HAL_OK) ) return HAL_OK;

    QSPI_Reset_Struct(QSPI_Memory);
    QSPI_Memory->sCommand->Instruction     = status;
    QSPI_Memory->sCommand->InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    QSPI_Memory->sCommand->AddressMode     = HAL_OSPI_ADDRESS_NONE;
    QSPI_Memory->sCommand->DataMode        = HAL_OSPI_DATA_NONE;
    QSPI_Memory->sCommand->NbData          = 0;

    HAL_OSPI_Command(QSPI_Memory->hospi, QSPI_Memory->sCommand, QSPI_TIMEOUT_GENERAL);

	if ( (status == QSPI_ENTER_SLEEP) && (check == HAL_TIMEOUT) ) return HAL_OK;
	if ( (status == QSPI_EXIT_SLEEP) && (check == HAL_OK) ) return HAL_OK;

	return HAL_ERROR;
}

/**
 * @brief Writes 8 bit sections of data to a page (256 bits) in the memory
 *
 * Corresponds to page 89 in the datasheet, Chapter 7.6.2
 * Requires QSPI / QE bit = 1
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 * @param *pData pointer to data (or array of data)
 * @param address 32bit address of where the data will start
 * @param size Number of bytes to write
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 *
 */
HAL_StatusTypeDef QSPI_Write_Page(QSPI_HandleTypeDef *QSPI_Memory, uint32_t address, uint8_t *pData, uint16_t size)
{
	if (QSPI_Command(QSPI_Memory, QSPI_WRITE_ENABLE) != HAL_OK) return HAL_ERROR;
	if (QSPI_Validate_Wel(QSPI_Memory, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;
	if (QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;

    QSPI_Memory->sCommand->Instruction     	= QSPI_WRITE_CMD;
    QSPI_Memory->sCommand->InstructionMode 	= HAL_OSPI_INSTRUCTION_1_LINE;
    QSPI_Memory->sCommand->Address		 	= address;
    QSPI_Memory->sCommand->AddressSize		= HAL_OSPI_ADDRESS_32_BITS;
    QSPI_Memory->sCommand->AddressMode     	= HAL_OSPI_ADDRESS_1_LINE;
    QSPI_Memory->sCommand->DummyCycles       = 0;
    QSPI_Memory->sCommand->DataMode        	= HAL_OSPI_DATA_4_LINES;
    QSPI_Memory->sCommand->NbData          	= size;

    if (HAL_OSPI_Command(QSPI_Memory->hospi, QSPI_Memory->sCommand, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;
    if (HAL_OSPI_Transmit(QSPI_Memory->hospi, pData, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;

    return QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_GENERAL);
}

/**
 * @brief Writes data contiguously to memory
 *
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 * @param data_size How much data to be written (in bytes)
 * @param *data Pointer to data to be stored
 * @param QSPI_Memory->occupied_data Pointer to data management variable
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 *
 */
HAL_StatusTypeDef QSPI_Write_Data(QSPI_HandleTypeDef *QSPI_Memory, QSPI_DataChunk_HandleTypeDef *DataBlock)
{
	if (QSPI_Command(QSPI_Memory, QSPI_WRITE_ENABLE) != HAL_OK) return HAL_ERROR;
    if (QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;

    // Cast the generic pointer to a byte pointer
    uint8_t *src = (uint8_t *)(DataBlock->tx_pData);
    uint32_t curAddress = QSPI_Memory->occupied_data;
    uint32_t DataLeft = DataBlock->data_size;

    while (DataLeft > 0)
    {
    	uint16_t DataBuffer = (DataLeft >= QSPI_PAGE_SIZE) ? QSPI_PAGE_SIZE : DataLeft; // Must be 16bit to store the 256 value, 8bit only goes up to 255
    	if (QSPI_Write_Page(QSPI_Memory, curAddress, src, DataBuffer) != HAL_OK) return HAL_ERROR;
    	// Increment counters and data pointer
    	src += DataBuffer;
    	curAddress += DataBuffer;
    	DataLeft -= DataBuffer;
    }
    // Sanity Check - if the final address is where it should be
    if (curAddress != (QSPI_Memory->occupied_data + DataBlock->data_size)) return HAL_ERROR;

    // Set addresses for data block management
    DataBlock->address_block_start = QSPI_Memory->occupied_data;
    DataBlock->address_block_end = curAddress;

    // Update Data Management variable to reflect occupied blocks
    QSPI_Memory->occupied_data += ((DataBlock->data_size + 4095) & ~4095);

	return HAL_OK;
}

/**
 * @brief Reads 8 bit sections of data from memory into a pointer
 *
 * Corresponds to page 24 in the datasheet, Chapter 7
 * Requires QSPI / QE bit = 1
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 * @param *pData pointer to data (or array of data) to store data in
 * @param address 32bit address of where the data will start
 * @param size Number of bytes to read
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 *
 */
HAL_StatusTypeDef QSPI_Read_Data(QSPI_HandleTypeDef *QSPI_Memory, QSPI_DataChunk_HandleTypeDef *DataBlock)
{
	if (QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;

    QSPI_Memory->sCommand->Instruction     	= QSPI_READ_CMD;
    QSPI_Memory->sCommand->InstructionMode 	= HAL_OSPI_INSTRUCTION_1_LINE;
    QSPI_Memory->sCommand->Address		 	= DataBlock->address_block_start;
    QSPI_Memory->sCommand->AddressSize		= HAL_OSPI_ADDRESS_32_BITS;
    QSPI_Memory->sCommand->AddressMode     	= HAL_OSPI_ADDRESS_4_LINES;
    QSPI_Memory->sCommand->DummyCycles       = QSPI_DUMMY_CYCLES_NUM;
    QSPI_Memory->sCommand->DataMode        	= HAL_OSPI_DATA_4_LINES;
    QSPI_Memory->sCommand->NbData          	= DataBlock->data_size;

    if (HAL_OSPI_Command(QSPI_Memory->hospi, QSPI_Memory->sCommand, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;
    if (HAL_OSPI_Receive(QSPI_Memory->hospi, DataBlock->rx_pData, QSPI_TIMEOUT_GENERAL) == HAL_OK) return HAL_OK; else return HAL_ERROR;
}

/**
 * @brief Erases blocks of memory equal to the data size
 *
 * Starts by erasing large amounts of memory (64kB chunks) until a finer resolution is required
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 * @param *data_size How much data to be read (in bytes)
 * @param address_start 32bit address of where the data will start to be written to
 * @param QSPI_Memory->occupied_data Pointer to data management variable
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 *
 */
HAL_StatusTypeDef QSPI_Erase_Data(QSPI_HandleTypeDef *QSPI_Memory, QSPI_DataChunk_HandleTypeDef *DataBlock)
{
	uint32_t curAddress = DataBlock->address_block_start;
	uint16_t NumBlocks = (DataBlock->data_size + 4095) >> 12;

//	while (NumBlocks > 16)
//	{
//		if (QSPI_Erase_64kB(QSPI_Memory->hospi, QSPI_Memory->sCommand, curAddress) != HAL_OK) return HAL_ERROR;
//		NumBlocks -= 16;
//		curAddress += (1 << 16);
//	}
//	while (NumBlocks > 8)
//	{
//		if (QSPI_Erase_32kB(QSPI_Memory->hospi, QSPI_Memory->sCommand, curAddress) != HAL_OK) return HAL_ERROR;
//		NumBlocks -= 8;
//		curAddress += (1 << 15);
//	}
	while (NumBlocks > 0)
	{
		if (QSPI_Erase_4kB(QSPI_Memory, curAddress) != HAL_OK) return HAL_ERROR;
		NumBlocks--;
		curAddress += (1 << 12);
	}
    // Update Data Management variable to reflect occupied blocks
    QSPI_Memory->occupied_data -= ((DataBlock->data_size + 4095) & ~4095);

    return HAL_OK;
}

/**
 * @brief Completes a write/read cycle to verify expected behaviour
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 * @param QSPI_Memory->occupied_data Pointer to data management variable
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 *
 */
uint32_t QSPI_Test_Reliability(QSPI_HandleTypeDef *QSPI_Memory)
{
    uint32_t test_size = (1<<10);
    uint8_t tx_buf[test_size];
    uint8_t rx_buf[test_size];
    uint32_t start_address = QSPI_Memory->occupied_data;

    QSPI_Erase_64kB(QSPI_Memory, start_address);

    // Create DataBlock
    QSPI_DataChunk_HandleTypeDef DataBlock;
    QSPI_Init_DataBlock(&DataBlock, test_size, tx_buf, rx_buf);

    // 1. Prepare dummy data (0, 1, 2, 3...)
    for (uint32_t i = 0; i < test_size; i++) tx_buf[i] = (uint8_t)(i % 256);

    // 2. Erase the area first
    if (QSPI_Erase_Data(QSPI_Memory, &DataBlock) != HAL_OK) return HAL_ERROR;

    // Reset occupied_data for the write test since Erase moved it back
    QSPI_Memory->occupied_data = start_address;

    // 3. Write the data
    if (QSPI_Write_Data(QSPI_Memory, &DataBlock) != HAL_OK) return HAL_ERROR;

    // 4. Read the data back
    if (QSPI_Read_Data(QSPI_Memory, &DataBlock) != HAL_OK) return HAL_ERROR;

    // 5. Verify integrity
    for (uint32_t i = 0; i < test_size; i++) {
        if (rx_buf[i] != tx_buf[i]) {
            return i+1;
        }
    }

    // 6. Clear data for cleanup
    if (QSPI_Erase_Data(QSPI_Memory, &DataBlock) != HAL_OK) return HAL_ERROR;

    return 0;
}

/**
 * @brief Erases a 4kB chunk of memory in a specified location
 *
 * Address MUST be aligned in 4kB segments e.g., 0x0000, 0x0001, etc.
 * Corresponds to page 94 in the datasheet, Chapter 7.6.6
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 * @param address 32bit address of where the data will start to be erased from
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 *
 */
HAL_StatusTypeDef QSPI_Erase_4kB(QSPI_HandleTypeDef *QSPI_Memory, uint32_t address)
{
	if (QSPI_Command(QSPI_Memory, QSPI_WRITE_ENABLE) != HAL_OK) return HAL_ERROR;
	if (QSPI_Validate_Wel(QSPI_Memory, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;

    QSPI_Memory->sCommand->Instruction     	= QSPI_ERASE_4KB;
    QSPI_Memory->sCommand->InstructionMode 	= HAL_OSPI_INSTRUCTION_1_LINE;
    QSPI_Memory->sCommand->Address		 	= address;
    QSPI_Memory->sCommand->AddressSize		= HAL_OSPI_ADDRESS_32_BITS;
    QSPI_Memory->sCommand->AddressMode     	= HAL_OSPI_ADDRESS_1_LINE;
    QSPI_Memory->sCommand->DummyCycles       = 0;
    QSPI_Memory->sCommand->DataMode        	= HAL_OSPI_DATA_NONE;
    QSPI_Memory->sCommand->NbData          	= 0;

    if (HAL_OSPI_Command(QSPI_Memory->hospi, QSPI_Memory->sCommand, QSPI_TIMEOUT_WIPE) != HAL_OK) return HAL_ERROR;

    return QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_WIPE);
}

/**
 * @brief Erases a 32kB chunk of memory in a specified location
 *
 * Address MUST be aligned in 4kB segments e.g., 0x0000, 0x0001, etc.
 * Corresponds to page 96 in the datasheet, Chapter 7.6.8
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 * @param address 32bit address of where the data will start to be erased from
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 *
 */
HAL_StatusTypeDef QSPI_Erase_32kB(QSPI_HandleTypeDef *QSPI_Memory, uint32_t address)
{
	if (QSPI_Command(QSPI_Memory, QSPI_WRITE_ENABLE) != HAL_OK) return HAL_ERROR;
	if (QSPI_Validate_Wel(QSPI_Memory, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;

    QSPI_Memory->sCommand->Instruction     	= QSPI_ERASE_32KB;
    QSPI_Memory->sCommand->InstructionMode 	= HAL_OSPI_INSTRUCTION_1_LINE;
    QSPI_Memory->sCommand->Address		 	= address;
    QSPI_Memory->sCommand->AddressSize		= HAL_OSPI_ADDRESS_32_BITS;
    QSPI_Memory->sCommand->AddressMode     	= HAL_OSPI_ADDRESS_1_LINE;
    QSPI_Memory->sCommand->DummyCycles       = 0;
    QSPI_Memory->sCommand->DataMode        	= HAL_OSPI_DATA_NONE;
    QSPI_Memory->sCommand->NbData          	= 0;

    if (HAL_OSPI_Command(QSPI_Memory->hospi, QSPI_Memory->sCommand, QSPI_TIMEOUT_WIPE) != HAL_OK) return HAL_ERROR;

    return QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_WIPE);
}

/**
 * @brief Erases a 64kB chunk of memory in a specified location
 *
 * Address MUST be aligned in 64kB segments
 * Corresponds to page 98 in the datasheet, Chapter 7.6.10
 *
 * @param *QSPI_Memory->hospi OCTOSPI hal handle.
 * @param *QSPI_Memory->sCommand HAL OSPI Regular Command Structure
 * @param address 32bit address of where the data will start to be erased from
 *
 * @retval Confirmation of operation, HAL_OK for successful, HAL_ERROR for unsuccessful
 *
 */
HAL_StatusTypeDef QSPI_Erase_64kB(QSPI_HandleTypeDef *QSPI_Memory, uint32_t address)
{
	if (QSPI_Command(QSPI_Memory, QSPI_WRITE_ENABLE) != HAL_OK) return HAL_ERROR;
	if (QSPI_Validate_Wel(QSPI_Memory, QSPI_TIMEOUT_GENERAL) != HAL_OK) return HAL_ERROR;

    QSPI_Memory->sCommand->Instruction     	= QSPI_ERASE_64KB;
    QSPI_Memory->sCommand->InstructionMode 	= HAL_OSPI_INSTRUCTION_1_LINE;
    QSPI_Memory->sCommand->Address		 	= address;
    QSPI_Memory->sCommand->AddressSize		= HAL_OSPI_ADDRESS_32_BITS;
    QSPI_Memory->sCommand->AddressMode     	= HAL_OSPI_ADDRESS_1_LINE;
    QSPI_Memory->sCommand->DummyCycles       = 0;
    QSPI_Memory->sCommand->DataMode        	= HAL_OSPI_DATA_NONE;
    QSPI_Memory->sCommand->NbData          	= 0;

    if (HAL_OSPI_Command(QSPI_Memory->hospi, QSPI_Memory->sCommand, QSPI_TIMEOUT_WIPE) != HAL_OK) return HAL_ERROR;

    return QSPI_Validate_Rdy(QSPI_Memory, QSPI_TIMEOUT_WIPE);
}
