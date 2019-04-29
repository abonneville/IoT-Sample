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

#include <errno.h> /* error codes */

#include "device.h"
#include "usbd_cdc_if.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"



/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/
extern USBD_HandleTypeDef hUsbDeviceFS;

static TaskHandle_t xHandlingTask;
static TaskHandle_t taskHandleNewUsbMessage;

static SemaphoreHandle_t txSemaphore;
static int32_t initWrite = 0;

static int32_t handleSet = 0;
static int32_t rxHandleSet = 0;
static uint32_t rxMessageLength = 0;


/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/


/**
 * @brief Device specific open for USB device
 * @param flags Indicate type of access requested (read, write, read/write)
 * @param mode Not used. When needed, provides additional options on top of the "flags" bit
 * @retval The new file descriptor(fd), or -1 if an error occurred (in which case,
 * 			errno is set appropriately).
 */
int usb1_open_r(struct _reent *ptr, int fd, int flags, int mode)
{
	return fd;
}


/**
 * @brief  Device specific close for USB interface
 * @param  ptr newlib re-entrance structure
 * @param  fd File descriptor, data stream to be disconnected
 * @retval On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
 */
int usb1_close_r(struct _reent *ptr, int fd)
{
	return 0;
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
_ssize_t usb1_write_r(struct _reent *ptr, int file, const void *buf, size_t len)
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
  * @param  buf: pointer to first character to be read
  * @parm   len: buffer capacity, in bytes
  * @retval -1 = error, zero or positive indicates how many bytes transferred
  */
_ssize_t usb1_read_r (struct _reent *ptr, int file, void *buf, size_t len)
{
	// USB driver has a fixed transfer request to the host of 64-bytes or less.
	// Since the USB driver has direct write access to the IO library buffer, need
	// to verify the buffer from the IO library can handle at least 64-bytes to
	// prevent buffer overrun.
	//
	// Note: future development could implement an additional intermediate buffer to isolate USB
	// driver from IO library buffer...but then it would no longer be a zero copy interface.
	if (len < 64 ) {
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
