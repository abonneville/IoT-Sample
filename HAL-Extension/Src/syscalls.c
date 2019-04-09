/*
 * syscalls.c
 *
 * Not a complete implementation, but provides definition for various STL and
 * related libraries as required.
 *
 *  Created on: Feb 23, 2019
 *      Author: Andrew
 */

#include <stdlib.h> // malloc() and free()
#include <limits.h>
#include <errno.h> // error codes
#include <sys/times.h> // used by _times()
#include <sys/time.h> // used by _gettimeofday()


#include "usbd_cdc_if.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// C Library runtime / system calls
//extern int _write(int file, char *ptr, int len);
//extern int _read (int file, char *buf, int count);
//extern void *malloc(size_t size);
//extern void free(void *ptr);

extern USBD_HandleTypeDef hUsbDeviceFS;

static TaskHandle_t xHandlingTask;
static TaskHandle_t taskHandleNewUsbMessage;

static SemaphoreHandle_t txSemaphore;
static int32_t initWrite = 0;

static int32_t handleSet = 0;
static int32_t rxHandleSet = 0;
static uint32_t rxMessageLength = 0;


static volatile uint8_t freeRTOSMemoryScheme = configUSE_HEAP_SCHEME; /* used by NXP thread aware debugger */


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
int _write(int file, char *buf, int len)
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
			errno = EBUSY;
			return -1;
		}

	}

	errno = ENOLCK;
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
int _read (int file, char *buf, int count)
{
	// USB driver has a fixed transfer request to the host of 64-bytes or less.
	// Since the USB driver has direct write access to the IO library buffer, need
	// to verify the buffer from the IO library can handle at least 64-bytes to
	// prevent buffer overrun.
	//
	// Note: future development could implement an additional intermediate buffer to isolate USB
	// driver from IO library buffer...but then it would no longer be a zero copy interface.
	if (count < 64 ) {
		errno = ENOBUFS;
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
int _gettimeofday( struct timeval *tv, void *tzvp )
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
void *malloc(size_t size)
{
	return pvPortMalloc(size);
}


/**
  * @brief	Deallocates a region of memory previously allocated by malloc or similar
  * @note	Redirect to use RTOS method for managing heap
  * @param  ptr to memory segment to be de-allocated and returned to heap
  */
void free(void *ptr)
{
	/* Force compiler to "keep" variable. */
	freeRTOSMemoryScheme = freeRTOSMemoryScheme;

	return vPortFree(ptr);
}
