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


#include "busArbitrator-I2C.h"

#include "stm32l4xx_hal.h"

#include "FreeRTOS.h"
#include "semphr.h"



/* Typedef -----------------------------------------------------------*/
typedef struct
{
	I2C_HandleTypeDef * i2cHandle;
	SemaphoreHandle_t semaphoreHandle;
	TaskHandle_t txTaskToNotify;
	TaskHandle_t rxTaskToNotify;
}BA_I2C_Handle_t;

/* Define ------------------------------------------------------------*/

/**
 * I2C does not specify a maximum time a slave can stretch the clock/data line, or maximum
 * time to indicate when a slave has stalled. As a guideline for this I2C implementation,
 * the SMBus specification will be referenced:
 *   - T low ext: 25mS max, Cumulative clock low extend time (slave device, clock stretching)
 */
#define T_timeout_mS 25
static const TickType_t maxBlockTime  = T_timeout_mS;
static const TickType_t maxAccessTime = T_timeout_mS + 1;


/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/
extern I2C_HandleTypeDef hi2c2;

static BA_I2C_Handle_t handles[NumberOfI2cBusses] = {};


/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/

/**
 * @brief Internal setup of bus arbitrator, call one-time for each bus being managed.
 * @param bus identifies which bus to be setup
 */
void BA_I2C_init( BA_I2C_descriptor_t bus )
{
	switch(bus)
	{
	case I2C2_Bus:
		if (handles[I2C2_Bus].semaphoreHandle == NULL)
		{
			handles[I2C2_Bus].i2cHandle = &hi2c2;
			handles[I2C2_Bus].semaphoreHandle = xSemaphoreCreateMutex();
			handles[I2C2_Bus].txTaskToNotify = NULL;
			handles[I2C2_Bus].rxTaskToNotify = NULL;
		}
		break;

	default:
		break;

	};
}



/**
 * @brief Read the contents of the requested device
 * @param bus Descriptor identifying which I2C bus to access
 * @param deviceAddr identifies which device to access
 * @param buffer:
 * 		inbound - content read from device
 * 		outbound - message to be sent to device before reading device content
 */
void BA_I2C_read( BA_I2C_descriptor_t bus, uint16_t deviceAddr, BA_I2C_buffer_t *buffer)
{

	if ( bus < NumberOfI2cBusses )
	{
		BA_I2C_Handle_t *hd = &handles[bus];

		if ( hd->semaphoreHandle != NULL )
		{
			if ( xSemaphoreTake( hd->semaphoreHandle, maxAccessTime ) == pdTRUE )
			{
				HAL_StatusTypeDef status;

				// Setup handle for task notification from ISR
				taskENTER_CRITICAL();
				hd->txTaskToNotify = xTaskGetCurrentTaskHandle(); // Used by the ISR to notify this task
				//xTaskNotifyStateClear(xHandlingTask); // Clear pending notifications state (does not affect notification value), from prior Tx attempts
				ulTaskNotifyTake( pdTRUE, 0); // Zero wait, clear both pending notification state & pending notification value, from prior Tx attempts

				/* Non-blocking: Send message requesting access starting at base register location */
				status = HAL_I2C_Master_Transmit_DMA( hd->i2cHandle, deviceAddr, buffer->outbound, buffer->outSize );
				taskEXIT_CRITICAL();

				if ( status == HAL_OK )
				{
					// Wait for TX Complete
					ulTaskNotifyTake( pdTRUE,  // Clear the notification value before exiting
										maxBlockTime );
				}

				// Setup handle for task notification from ISR
				taskENTER_CRITICAL();
				hd->rxTaskToNotify = xTaskGetCurrentTaskHandle(); // Used by the ISR to notify this task
				//xTaskNotifyStateClear(xHandlingTask); // Clear pending notifications state (does not affect notification value), from prior Tx attempts
				ulTaskNotifyTake( pdTRUE, 0); // Zero wait, clear both pending notification state & pending notification value, from prior Tx attempts

				/* Non-blocking Read contents from device */
				status = HAL_I2C_Master_Receive_DMA( hd->i2cHandle, deviceAddr, buffer->inbound, buffer->inSize );
				taskEXIT_CRITICAL();

				if ( status == HAL_OK )
				{
					// Wait for RX Complete
					ulTaskNotifyTake( pdTRUE,  // Clear the notification value before exiting
										maxBlockTime );
				}

				xSemaphoreGive(hd->semaphoreHandle);
			}
		}
	}
}


/**
 * @brief Write data to the requested device
 * @param bus Descriptor identifying which I2C bus to access
 * @param deviceAddr identifies which device to access
 * @param buffer:
 * 		inbound - not used
 * 		outbound - message and data to be sent to device
 */
void BA_I2C_write( BA_I2C_descriptor_t bus, uint16_t deviceAddr, BA_I2C_buffer_t *buffer)
{
	if ( bus < NumberOfI2cBusses )
	{
		BA_I2C_Handle_t * hd = &handles[bus];

		if ( hd->semaphoreHandle != NULL )
		{
			if ( xSemaphoreTake( hd->semaphoreHandle, maxAccessTime ) == pdTRUE )
			{
				// Setup handle for task notification from ISR
				taskENTER_CRITICAL();
				hd->txTaskToNotify = xTaskGetCurrentTaskHandle(); // Used by the ISR to notify this task
				//xTaskNotifyStateClear(xHandlingTask); // Clear pending notifications state (does not affect notification value), from prior Tx attempts
				ulTaskNotifyTake( pdTRUE, 0); // Zero wait, clear both pending notification state & pending notification value, from prior Tx attempts

				/* Non-blocking: Write value(s) starting at requested register location */
				HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_DMA( hd->i2cHandle, deviceAddr, buffer->outbound, buffer->outSize );
				taskEXIT_CRITICAL();

				if ( status == HAL_OK )
				{
					// Wait for TX Complete
					ulTaskNotifyTake( pdTRUE,  // Clear the notification value before exiting
										maxBlockTime );
				}
				
				xSemaphoreGive(hd->semaphoreHandle);

			}
		}
	}
}


/**
  * @brief  Master Tx Transfer completed callback.
  * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @retval None
  */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	/* Find matching bus handle */
	for ( size_t index = 0; index < NumberOfI2cBusses; index++ )
	{
		if ( handles[index].i2cHandle == hi2c )
		{
			/* Match found */
			configASSERT( handles[index].txTaskToNotify != NULL );

			vTaskNotifyGiveFromISR( handles[index].txTaskToNotify,
							   &xHigherPriorityTaskWoken );

			/* There is no transmission in progress, so no tasks to notify. */
			handles[index].txTaskToNotify = NULL;

			break;
		}
	}

	/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
	should be performed to ensure the interrupt returns directly to the highest
	priority task.  The macro used for this purpose is dependent on the port in
	use and may be called portEND_SWITCHING_ISR(). */
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/**
  * @brief  Master Rx Transfer completed callback.
  * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @retval None
  */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	/* Find matching bus handle */
	for ( size_t index = 0; index < NumberOfI2cBusses; index++ )
	{
		if ( handles[index].i2cHandle == hi2c )
		{
			/* Match found */
			configASSERT( handles[index].rxTaskToNotify != NULL );

			vTaskNotifyGiveFromISR( handles[index].rxTaskToNotify,
							   &xHigherPriorityTaskWoken );

			/* There is no reception in progress, so no tasks to notify. */
			handles[index].rxTaskToNotify = NULL;

			break;
		}

	}

	/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
	should be performed to ensure the interrupt returns directly to the highest
	priority task.  The macro used for this purpose is dependent on the port in
	use and may be called portEND_SWITCHING_ISR(). */
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

