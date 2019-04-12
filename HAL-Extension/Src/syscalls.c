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


#include <stdlib.h> // malloc() and free()
#include <limits.h>
#include <errno.h> // error codes
#include <sys/times.h> // used by _times()
#include <sys/time.h> // used by _gettimeofday()
#include <_newlib_version.h>
#include <reent.h> /* Reentrant versions of system calls.  */

#include "usbd_cdc_if.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


/**
 * Newlib references:
 * https://sourceware.org/newlib/
 * http://www.nadler.com/embedded/newlibAndFreeRTOS.html
 * http://www.billgatliff.com/newlib.html
 * https://www.cs.ccu.edu.tw/~pahsiung/courses/esd/resources/newlib.pdf
 */



extern USBD_HandleTypeDef hUsbDeviceFS;

static TaskHandle_t xHandlingTask;
static TaskHandle_t taskHandleNewUsbMessage;

static SemaphoreHandle_t txSemaphore;
static int32_t initWrite = 0;

static int32_t handleSet = 0;
static int32_t rxHandleSet = 0;
static uint32_t rxMessageLength = 0;

size_t xPortGetHeapBlockSize( void *pv );

static volatile uint8_t freeRTOSMemoryScheme = configUSE_HEAP_SCHEME; /* used by NXP thread aware debugger */




/**
 * Hooks to setup thread specific buffers for stdio. These need to be called from the
 * running thread, so the Newlib reentrant structure is initialized for the specific
 * thread.
 */
void SetUsbTxBuffer(void)
{
	// Redirect IO library to use buffers allocated by USB driver
	setvbuf(stdout, (char *)UserTxBufferFS, _IOFBF, APP_TX_DATA_SIZE);
}

void SetUsbRxBuffer(void)
{
	// Redirect IO library to use buffers allocated by USB driver
	setvbuf(stdin,  (char *)UserRxBufferFS, _IOLBF, APP_RX_DATA_SIZE);
}


/**
  * @brief  Redirects message out USB. Method blocks until transfer fully completes or times out.
  * @note   Zero copy, buffer is handed off directly to USB driver
  * 		Known limitations:
  * 			1.) If link with host is down, the transfer will fail.
  * 			2.) 1-byte transfers (cerr, clog, etc...) will send just 1-byte and then block
  * @param  file: not used
  * @param  buf: pointer to first character to be sent
  * @parm   len: how many characters to be sent
  * @retval -1 = message not sent or timed out waiting
  * 		zero or positive indicates how many bytes transferred
  */
_ssize_t _write_r(struct _reent *ptr, int file, const void *buf, size_t len)
{
	file = file; //not used, suppress compiler warning

	uint32_t txResult = 0;

	// What to do when len is zero:
	// For USB, zero length packets are used to indicate end of transfer. For the C++ STL, zero
	// means to flush data buffers as required. So a zero here is not related to USB end of
	// transmission and will be ignored.
	if (len == 0) return 0; // avoid unnecessary call to USB driver

	// To achieve zero-copy, means we have to block until transfer completes before releasing buffer
	// back to application. Calculate a nominal transfer/block time based on the following
	// assumptions:
	// 		1. Average 'n' data packets per frame
	//		2. Host honors requested polling interval
	uint32_t transferTime = (len + 63) >> 6; // determine number of packets in message, assume 1 packet per frame
	transferTime += CDC_FS_BINTERVAL; // add in polling interval requested from host
	TickType_t xMaxBlockTime = pdMS_TO_TICKS(transferTime);

	// By design, a single thread handles normal transmit of messages. However, when debugging it can
	// be useful to allow other threads to send debug messages as well. To support multiple threads,
	// a semaphore has been added to guard access to the USB transmit capability.
	if (initWrite == 0) {
		initWrite = 1;
		txSemaphore = xSemaphoreCreateMutex();
		vQueueAddToRegistry(txSemaphore, __func__);

	}

	if (txSemaphore != NULL) {
		if (xSemaphoreTake(txSemaphore, xMaxBlockTime + 1) == pdTRUE)
		{
			// Setup handle for task notification from ISR
			taskENTER_CRITICAL();
			xHandlingTask = xTaskGetCurrentTaskHandle(); // Used by the ISR to notify this task
			//xTaskNotifyStateClear(xHandlingTask); // Clear pending notifications state (does not affect notification value), from prior Tx attempts
			ulTaskNotifyTake( pdTRUE, 0); // Zero wait, clear both pending notification state & pending notification value, from prior Tx attempts
			handleSet = 1; // Flag to notify ISR handle is now set for notification

			// Initiate TX
			USBD_StatusTypeDef status = (USBD_StatusTypeDef)CDC_Transmit_FS((uint8_t *)buf, len);
			taskEXIT_CRITICAL();

			if (status == USBD_OK) {
				// Wait for TX Complete
				txResult = ulTaskNotifyTake( pdTRUE,  // Clear the notification value before exiting
								  xMaxBlockTime );


				if (txResult != 0) {
					xSemaphoreGive(txSemaphore);
					return len;
				}
			}

			xSemaphoreGive(txSemaphore);
			ptr->_errno = EBUSY;
			return -1;
		}

	}

	ptr->_errno = ENOLCK;
	return -1;
}



/**
  * @brief  When USB TX complete event occurs, this method notifies the RTOS, which
  * 		in turns notifies the blocked task waiting for USB TX complete
  */
void SYS_CDC_TxCompleteIsr(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;


	// TX completed, notify task
	if (handleSet == 1) {
		handleSet = 0;
		vTaskNotifyGiveFromISR( xHandlingTask,
						   &xHigherPriorityTaskWoken );
	}

	/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
	should be performed to ensure the interrupt returns directly to the highest
	priority task.  The macro used for this purpose is dependent on the port in
	use and may be called portEND_SWITCHING_ISR(). */
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}


/**
  * @brief  Attempts to read up to count bytes from USB message into the buffer starting at buf.
  * @note   Zero copy, buffer is handed off directly to USB driver
  * @note   This is a blocking call with no timeout.
  * @param  file: not used
  * @param  buf: pointer to first character to be written
  * @parm   count: buffer capacity, in bytes
  * @retval -1 = error, zero or positive indicates how many bytes transferred
  */
_ssize_t _read_r (struct _reent *ptr, int file, void *buf, size_t count)
{
	// USB driver has a fixed transfer request to the host of 64-bytes or less.
	// Since the USB driver has direct write access to the IO library buffer, need
	// to verify the buffer from the IO library can handle at least 64-bytes to
	// prevent buffer overrun.
	//
	// Note: future development could implement an additional intermediate buffer to isolate USB
	// driver from IO library buffer...but then it would no longer be a zero copy interface.
	if (count < 64 ) {
		ptr->_errno = ENOBUFS;
		return -1;
	}

	// Setup handle for task notification from ISR
	taskENTER_CRITICAL();
	taskHandleNewUsbMessage = xTaskGetCurrentTaskHandle(); // Used by the ISR to notify this task
	//xTaskNotifyStateClear(xHandlingTask); // Clear pending notifications state (does not affect notification value), from prior Tx attempts
	ulTaskNotifyTake( pdTRUE, 0); // Zero wait, clear both pending notification state & pending notification value, from prior Tx attempts
	rxHandleSet = 1; // Flag to notify ISR handle is now set for notification

	// Hand buffer to USB driver...notifies PC host clear to send more data
	USBD_CDC_SetRxBuffer(&hUsbDeviceFS, (uint8_t *)buf);
	USBD_CDC_ReceivePacket(&hUsbDeviceFS);
	taskEXIT_CRITICAL();

	// Now wait for the USB host to send us something
	ulTaskNotifyTake(pdTRUE,  // Clear the notification value before exiting
					 portMAX_DELAY ); // wait forever

	// Message received, return number of bytes in message
	return rxMessageLength;
}


/**
  * @brief  When USB RX message event occurs, this method notifies the RTOS, which
  * 		in turns notifies the blocked task waiting for USB RX message RX event.
  * @parm   length: how many bytes in message
  */
void SYS_CDC_RxMessageIsr(uint32_t length)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;


	rxMessageLength = length;

	// RX message event, notify task
	if (rxHandleSet == 1) {
		vTaskNotifyGiveFromISR( taskHandleNewUsbMessage,
						   &xHigherPriorityTaskWoken );
	}

	/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
	should be performed to ensure the interrupt returns directly to the highest
	priority task.  The macro used for this purpose is dependent on the port in
	use and may be called portEND_SWITCHING_ISR(). */
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}


#if 0 // not used, untested
/**
  * @brief
  * @note
  * @param  tms: not supported,
  * @retval 32-bit unsigned value
  */
clock_t _times(struct tms *buf)
{
	clock_t sysTickCount = xTaskGetTickCount();
	buf->tms_utime = buf->tms_stime = buf->tms_cutime = buf->tms_cstime = sysTickCount;
	return sysTickCount;
}
#endif


/**
  * @brief	Used by Google Test, not to be used anywhere else
  * @note	POSIX.1-2008 marks gettimeofday() as obsolete
  * @param  *tv is set to the time since startup
  * @param	*tzvp, obsolete timezone value, place holder for legacy interface
  * @retval 0 for success, or -1 for failure (in which case errno is set appropriately
  */
int _gettimeofday_r(struct _reent *ptr, struct timeval *tv, void *tzvp)
{
	time_t t = xTaskGetTickCount();  // get uptime in ticks
	tv->tv_sec  = ( t / configTICK_RATE_HZ );  // convert to seconds
	tv->tv_usec = ( t % configTICK_RATE_HZ ) * 1000;  // get remaining microseconds
	return 0;
}


/**
  * @brief	Allocates a region of memory large enough to hold an object of "size"
  * @note	Redirect to use RTOS method for managing heap
  * @param  size amount of memory to be allocated (as measured by sizeof operator)
  * @retval NULL unable to allocate requested memory, otherwise non-NULL pointer to
  * first element in the allocated memory region.
  */
void *_malloc_r(struct _reent *ptr, size_t size)
{
	return pvPortMalloc(size);
}


void *_realloc_r (struct _reent *ptr, void *old_ptr, size_t new_size)
{
	  void *new_ptr = pvPortMalloc (new_size);

	  if (old_ptr && new_ptr)
	    {
	      size_t old_size = xPortGetHeapBlockSize(old_ptr);

	      size_t copy_size = old_size > new_size ? new_size : old_size;
	      memcpy (new_ptr, old_ptr, copy_size);
	      vPortFree (old_ptr);
	    }

	  return new_ptr;
}


/**
  * @brief	Allocates a region of memory large enough to hold "n" objects of "size"
  * 		Prior to returning, memory is initialized to zero.
  * @note	Redirect to use RTOS method for managing heap
  * @param  count is how many objects of type size to be allocated
  * @param  size of the object to be allocated (as measured by sizeof operator)
  * @retval NULL unable to allocate requested memory, otherwise non-NULL pointer to
  * first element in the allocated memory region.
  */
void *_calloc_r(struct _reent *ptr, size_t count, size_t size)
{
	size_t block = count * size;

	void *ret = pvPortMalloc(block);
	if (ret != NULL)
		memset(ret, 0, block);

	return ret;
}


/**
  * @brief	Deallocates a region of memory previously allocated by malloc or similar
  * @note	Redirect to use RTOS method for managing heap
  * @param  block or memory segment to be de-allocated and returned to heap
  */
void _free_r(struct _reent *ptr, void *block)
{
	/* Force compiler to "keep" variable. */
	freeRTOSMemoryScheme;

	return vPortFree(block);
}


/**
  * @brief Required by Newlib C runtime to allocate memory during library initialization.
  * @note  Compliant stub. See linker command file to set memory region used here
  * @param  incr amount of memory to be allocated/deallocated in bytes
  * @retval -1 unable to allocate/deallocate requested memory
  * 		pointer to first element in the new allocated memory region.
  */
#define SBRK_SIZE (0)
static char sbrkHeap[SBRK_SIZE];
const char *const heapLimit = &sbrkHeap[SBRK_SIZE];
void *_sbrk_r(struct _reent *ptr, ptrdiff_t incr)
{
	static char *heap_end = sbrkHeap;
	char *prev_heap_end;

	vTaskSuspendAll();
	prev_heap_end = heap_end;
	if (heap_end + incr > heapLimit)
	{
		ptr->_errno = ENOMEM;
		xTaskResumeAll();
		return (void *) -1;
	}

	heap_end += incr;

	xTaskResumeAll();
	return (void *) prev_heap_end;
}
