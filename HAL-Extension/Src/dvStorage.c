/*
 * Copyright (C) 2019 Andrew Bonneville.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <fcntl.h> /* file IO, flag and mode bits*/
#include <errno.h> /* error codes */
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "device.h"
#include "stm32l4xx_hal.h"

/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/
/* Place "storage" at end of on-chip flash memory */
#define STORAGE_SIZE (3 * FLASH_PAGE_SIZE)
#define FLASH_USER_START_ADDR   (FLASH_BASE + FLASH_SIZE - STORAGE_SIZE)
#define FLASH_USER_END_ADDR     (FLASH_USER_START_ADDR + STORAGE_SIZE - 1)


/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/
/* Offset to the next available access location */
static uint32_t positionIndicator = 0;
static uint32_t accessMode = 0;

/* Function prototypes -----------------------------------------------*/
static uint32_t GetBank(uint32_t Addr);
static uint32_t GetPage(uint32_t Addr);
static _ssize_t EraseAllStorage(void);
static _ssize_t WriteAllStorage(const void *buf, size_t len);


/* External functions ------------------------------------------------*/


/**
 * @brief Device specific open for configuration parameters
 * @param flags Indicate type of access requested (read, write, read/write)
 * @param mode Not used. When needed, provides additional options on top of the "flags" bit
 * @retval The new file descriptor(fd), or -1 if an error occurred (in which case,
 * 			errno is set appropriately).
 */
int storage_open_r(struct _reent *ptr, int fd, int flags, int mode)
{
	vTaskSuspendAll();
	positionIndicator = 0;
	accessMode = flags & O_ACCMODE;
	xTaskResumeAll();
	return fd;
}


/**
 * @brief  Device specific close for configuration parameters
 * @param  ptr newlib re-entrance structure
 * @param  fd File descriptor, data stream to be disconnected
 * @retval On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
 */
int storage_close_r(struct _reent *ptr, int fd)
{
	return 0;
}


/**
  * @brief  Write buffer contents until memory is full or buffer is emptied.
  * @param  fd: not used, file descriptor
  * @param  buf: pointer to first character to be written
  * @parm   len: how many characters to be written
  * @retval -1 = message not sent or timed out waiting
  * 		zero or positive indicates how many bytes transferred
  */
_ssize_t storage_write_r(struct _reent *ptr, int fd, const void *buf, size_t len)
{
	_ssize_t status = 0;

	/* Unlock the Flash to enable the flash control register access */
	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

	/* Erase old data */
	if (positionIndicator == 0) {
		status = EraseAllStorage();
	}

	/* Write new data */
	if (status == 0) {
		status = WriteAllStorage(buf, len);
	}

	/* Lock the Flash to disable the flash control register access */
	HAL_FLASH_Lock();

	if (status < 0) {
		ptr->_errno = EIO;
		return -1;
	}
	else if (status == 0) {
		ptr->_errno = ENOSPC;
		return -1;
	}

	return (status < len ? status : len);
}


/**
  * @brief  Attempts to read up to count bytes from "storage" into a buffer.
  * @param  fd: not used, file descriptor
  * @param  buf: destination pointer to transfer data from "storage"
  * @parm   len: buffer capacity, in bytes
  * @retval -1 = error, zero or positive indicates how many bytes transferred
  */
_ssize_t storage_read_r (struct _reent *ptr, int fd, void *buf, size_t len)
{
	size_t count = 0;
	uint8_t *lhs = (uint8_t *)buf;
	const uint8_t *rhs = (uint8_t *)(FLASH_USER_START_ADDR + positionIndicator);

	/* Read data into buffer */
	while (( rhs < (uint8_t *)FLASH_USER_END_ADDR) &&
			(count < len)) {
		lhs[count] = rhs[count];
		count++;
	}

	positionIndicator += count;

	return count;
}



/**
 * @brief Destroy contents of memory region by erasing flash memory
 * @retval On success, zero is returned. On error, -1 is returned
 */
static _ssize_t EraseAllStorage(void)
{
	uint32_t FirstPage = GetPage(FLASH_USER_START_ADDR);
	uint32_t NbOfPages = GetPage(FLASH_USER_END_ADDR) - FirstPage + 1;
	uint32_t BankNumber = GetBank(FLASH_USER_START_ADDR);
	uint32_t PAGEError = 0;

	FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Banks       = BankNumber;
	EraseInitStruct.Page        = FirstPage;
	EraseInitStruct.NbPages     = NbOfPages;

	/* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
	 * you have to make sure that these data are rewritten before they are accessed during code
	 * execution. If this cannot be done safely, it is recommended to flush the caches by setting the
	 * DCRST and ICRST bits in the FLASH_CR register.
	 */
	HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
	return (status == HAL_OK ? 0 : -1);
}


/**
 * @brief  Write buffer contents until storage is full or buffer is emptied.
 * @note   The STM32L4xx part uses an 8-bit ECC per row (8-bytes, double word), which means once
 * 		   a value is written to a row, the ECC is set and the contents of the row cannot be changed.
 * @param  buf: pointer to first character to be written
 * @parm   len: how many characters to be written
 * @retval How many characters written into memory.
 */
static _ssize_t WriteAllStorage(const void *buf, size_t len)
{
	HAL_StatusTypeDef status = HAL_ERROR;
	uint32_t Address = FLASH_USER_START_ADDR + positionIndicator;
	uint64_t *data = (uint64_t *)buf;
	uint32_t count = 0;

	/* Write up to and including the final byte allocated for storage.
	 * Limitations:
	 *		- Requires all writes to be 8-bytes, except the very last write can be any length
	 *		- If last write is less than 8-bytes, the remaining bytes written are garbage values
	 *		- If buf is not 4/8-byte aligned, a hard fault will occur when attempting to call
	 *		HAL. This will not happen if setvbuf enables buffering, however, if the user
	 *		should set it to no buffering then its likely that a buffer pointer will
	 *		eventually be mis-aligned. Note, with buffering off, the user will be writing
	 *		1-byte for every 8-byte row of flash, its not logical or feasible to support turning
	 *		off the buffering at this time.
	 */
	while (( Address < FLASH_USER_END_ADDR) &&
			(count < len)) {
		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, Address, *data);
		if (status != HAL_OK) break;

		Address += 8;
		count += 8;
		++data;
	}

	count = (count < len ? count : len);
	positionIndicator += count;

	return count;
}



/**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t GetPage(uint32_t Addr)
{
   uint32_t page = 0;

  if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
  {
    /* Bank 1 */
    page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
  }
  else
  {
    /* Bank 2 */
    page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
  }

  return page;
}

/**
  * @brief  Gets the bank of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The bank of a given address
  */
static uint32_t GetBank(uint32_t Addr)
{
  uint32_t bank = 0;

  if (READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE) == 0)
  {
  	/* No Bank swap */
    if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
    {
      bank = FLASH_BANK_1;
    }
    else
    {
      bank = FLASH_BANK_2;
    }
  }
  else
  {
  	/* Bank swap */
    if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
    {
      bank = FLASH_BANK_2;
    }
    else
    {
      bank = FLASH_BANK_1;
    }
  }

  return bank;
}


/**
 * @brief  Calculates a 32-bit CRC over a variable length message.
 * @note   Algorithm adapted from Christopher Kormanyos book, "Real-Time C++"
 * @first  Starting location for calculating checksum
 * @last   Location to stop calculating checksum, location is not part of the CRC result
 * @retval the 32-bit CRC value
 *
 * Name: CRC-32/MPEG-2
 * Polynomial: 0x04C1 1DB7
 * Initial Value: 0xFFFF FFFF
 * Test input '1'...'9' : result 0x0376 E6E7
 */
const uint32_t table[16] = {
		0x00000000, 0x04C11DB7,
		0x09823B6E, 0x0D4326D9,
		0x130476DC, 0x17C56B6B,
		0x1A864DB2, 0x1E475005,
		0x2608EDB8, 0x22C9F00F,
		0x2F8AD6D6, 0x2B4BCB61,
		0x350C9B64, 0x31CD86D3,
		0x3C8EA00A, 0x384FBDBD
};

uint32_t crc_mpeg2(uint8_t *first, uint8_t *last)
{
	uint32_t crc = UINT32_C(0xFFFFFFFF);

	while (first != last) {
		const uint_fast8_t value = (*first) & UINT8_C(0xFF);
		const uint_fast8_t byte  = value;

		uint_fast8_t index;

		/* Perform the CRC-32/MPEG-2 algorithm */
		index = ( ( (uint_fast8_t)(crc >> 28) )
				^ ( (uint_fast8_t)(byte >> 4) )
				) & UINT8_C(0x0F);

		crc = ((uint32_t) ( (uint32_t)(crc << 4)
			              & UINT32_C(0xFFFFFFF0) ) )
						  ^ table[index];

		index = ( ( (uint_fast8_t)(crc >> 28) )
				^ ( (uint_fast8_t)(byte     ) )
				) & UINT8_C(0x0F);

		crc = ((uint32_t) ( (uint32_t)(crc << 4)
			              & UINT32_C(0xFFFFFFF0) ) )
						  ^ table[index];

		++first;
	}

	return crc;
}
