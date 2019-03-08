/*
 * syscalls.c
 *
 * Not a complete implementation, but provides definition for various STL and
 * related libraries as required.
 *
 *  Created on: Feb 23, 2019
 *      Author: Andrew
 */

#include <limits.h>
#include "usbd_cdc_if.h"
#include "FreeRTOS.h"
#include "task.h"

extern int _write(int file, char *ptr, int len);

static TaskHandle_t xHandlingTask;


static int32_t handleSet = 0;

/**
  * @brief  Redirects message out USB. Method blocks until transfer fully completes or times out.
  * @note   Zero copy, buffer is handed off directly to USB driver
  * 		Known limitations:
  * 			1.) If link with host is down, the transfer will fail.
  * 			2.) 1-byte transfers (cerr, clog, etc...) will send just 1-byte and then block
  * @param  file: not used
  * @param  buf: pointer to first character to be sent
  * @parm   len: how many characters to be sent
  * @retval -1 = message not sent, zero or positive indicates how many bytes transferred
  */
int _write(int file, char *buf, int len)
{
	file = file; //not used, suppress compiler warning

	uint32_t txResult = 0;

	// To achieve zero-copy, means we have to block until transfer completes before releasing buffer
	// back to application. Calculate a nominal transfer/block time based on the following
	// assumptions:
	// 		1. Average 'n' data packets per frame
	//		2. Host honors requested polling interval
	// TODO finalize value to be used, 500mS, 5000mS, or dynamic value based on size of message
	uint32_t transferTime = (len + 63) >> 6; // determine number of packets in message, assume 1 packet per frame
	//transferTime = (transferTime + 1) >> 1; // determine number of frames, assume 'n' packets per frame
	transferTime += CDC_FS_BINTERVAL; // add in polling interval requested from host
	TickType_t xMaxBlockTime = pdMS_TO_TICKS(transferTime);


	// What to do when len is zero:
	// For USB, zero length packets are used to indicate end of transfer. For the C++ STL, zero
	// means to flush data buffers as required. So a zero here is not related to USB end of
	// transmission and will be ignored.
	if (len == 0) return 0; // avoid unnecessary call to USB driver

	// Known limitations with calling CDC_Transmit_FS()
	// 1.) If link with host is down, the transfer will fail. User will then need to
	//		periodically call cout.fail() and cout.clear() to maintain/respond to lost link with host
	// 2.) 1-byte transfers (cerr, clog, etc...) will send just 1-byte and then block until transfer
	//		is complete.
	//

	// Setup handle for task notification from ISR
	taskENTER_CRITICAL();
	xHandlingTask = xTaskGetCurrentTaskHandle(); // Used by the ISR to notify this task
	//xTaskNotifyStateClear(xHandlingTask); // Clear pending notifications state (does not affect notification value), from prior Tx attempts
	ulTaskNotifyTake( pdTRUE, 0); // Zero wait, clear both pending notification state & pending notification value, from prior Tx attempts
	handleSet = 1; // Flag to notify ISR handle is now set for notification

	// Initiate TX
	USBD_StatusTypeDef status = (USBD_StatusTypeDef)CDC_Transmit_FS((uint8_t *)buf, len);

	if (status == USBD_OK) {
		// Wait for TX Complete
		taskEXIT_CRITICAL();
		txResult = ulTaskNotifyTake( pdTRUE,  // Clear the notification value before exiting
						  xMaxBlockTime );
	}
	else {
		taskEXIT_CRITICAL();
	}

	if ((status == USBD_OK) && (txResult != 0)) {
		return len;
	}

	return -1;
}



/**
  * @brief  When USB TX complete event occurs, this method notifies the RTOS, which
  * 		in turns notifies the blocked task waiting for USB TX complete
  */
void SYS_CDC_TxCompleteIsr(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;


	// TX completed, notify task by setting TX_Complete bit
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

