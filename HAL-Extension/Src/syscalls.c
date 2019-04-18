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


#include <stdlib.h> /* malloc() and free() */
#include <limits.h>
#include <errno.h> /* error codes */
#include <sys/times.h> /* used by _times() */
#include <sys/time.h> /* used by _gettimeofday() */
#include <sys/stat.h> /* fstat */
#include <_newlib_version.h>
#include <reent.h> /* Reentrant versions of system calls.  */
#include <fcntl.h> /* file IO, flag and mode bits*/
#include <unistd.h> /* file descriptors stdin, stdout, stderr */
#include <assert.h> /* compile time assertions */


#include "syscalls.h"
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

typedef struct {
   const char *name;
   size_t nameSize;
   int (*_open_r  )( struct _reent *r, int fd, int flags, int mode );
   int (*_close_r )( struct _reent *r, int fd );
   _ssize_t (*_write_r )( struct _reent *r, int fd, const void *ptr, size_t len );
   _ssize_t (*_read_r  )( struct _reent *r, int fd, void *ptr, size_t len );
} DeviceOperations_t;


typedef enum {
	fdInvalid = -1,
	std_in = STDIN_FILENO,
	std_out = STDOUT_FILENO,
	std_err = STDERR_FILENO
} FileDescriptor_t;


const Device_t Device = {.std_in = "std_in", .std_out = "std_out", .std_err = "std_err"};

static int usb1_open_r(struct _reent *ptr, int fd, int flags, int mode);
static int usb1_close_r(struct _reent *ptr, int fd);
static _ssize_t usb1_write_r(struct _reent *ptr, int file, const void *buf, size_t len);
static _ssize_t usb1_read_r (struct _reent *ptr, int file, void *buf, size_t count);
static const DeviceOperations_t dvStdin = {
		Device.std_in, sizeof(Device.std_in),
		usb1_open_r,
		usb1_close_r,
		usb1_write_r,
		usb1_read_r
};

static const DeviceOperations_t dvStdout = {
		Device.std_out, sizeof(Device.std_out),
		usb1_open_r,
		usb1_close_r,
		usb1_write_r,
		usb1_read_r
};

static const DeviceOperations_t dvStderr = {
		Device.std_err, sizeof(Device.std_err),
		usb1_open_r,
		usb1_close_r,
		usb1_write_r,
		usb1_read_r
};

/**
 * The order in which devices are added to the DeviceList, corresponds to its "file descriptor" (fd)
 * value used by the newlib library. Newlib pre-defines the first three fd, additional files/devices
 * can be added up to FOPEN_MAX.
 */
const DeviceOperations_t *DeviceList[] = {
		&dvStdin, /* fd = [0], STDIN_FILENO */
		&dvStdout, /* fd = [1], STDOUT_FILENO */
		&dvStderr, /* fd = [2], STDERR_FILENO */
		0 /* terminates the list */
};

static_assert((sizeof(DeviceList) / sizeof(intptr_t)) - 1  <= FOPEN_MAX, "Error: device list exceeds newlibs internal list for open streams");


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
 * @brief  Initializes IO buffer space used at the application layer.
 * @note   Device/file must be opened prior to calling this method
 * @param  handle For buffer/stream to be setup
 * @retval none
 */
void app_SetBuffer(FILE *handle)
{
	if (handle == NULL) return;

	switch ((FileDescriptor_t)handle->_file) {
	case std_in:
		setvbuf(stdin,  (char *)UserRxBufferFS, _IOLBF, APP_RX_DATA_SIZE);
		break;

	case std_out:
		setvbuf(stdout, (char *)UserTxBufferFS, _IOFBF, APP_TX_DATA_SIZE);
		break;

	case std_err:
		/* use defaults defined by newlib library */
		break;

	default:
		break;
	};

}


/**
 * @brief Implements a routing algorithm between newlib library and multiple devices. See
 * the device specific methods for details.
 * @note  This applies to _open_r, _close_r, _write_r, and _read_r
 *
 */
int _open_r(struct _reent *ptr, const char *name, int flags, int mode)
{
	   int index = 0;
	   int fd = -1;

	   /* Set file descriptor based on provided name */
	   do {
	      if( memcmp( name, DeviceList[index]->name, DeviceList[index]->nameSize ) == 0 ) {
	         fd = index;
	         break;
	      }
	   } while( DeviceList[index++] );

	   if( fd != -1 ) {
		   /* Found a match, attempt to open device */
		   fd = DeviceList[fd]->_open_r( ptr, fd, flags, mode );
	   }
	   else {
		   ptr->_errno = ENODEV;
	   }

	   return fd;
}

int _close_r(struct _reent *ptr, int fd)
{
	return DeviceList[fd]->_close_r( ptr, fd );
}

_ssize_t _write_r(struct _reent *ptr, int fd, const void *buf, size_t len)
{
	return DeviceList[fd]->_write_r( ptr, fd, buf, len );
}

_ssize_t _read_r (struct _reent *ptr, int fd, void *buf, size_t len)
{
	return DeviceList[fd]->_read_r( ptr, fd, buf, len );
}


/**
 * @brief  Status of an open file/device.
 * @note   Minimal implementation, all devices will be reported as special character device
 * @param  ptr newlib re-entrance structure
 * @param  fd File descriptor, identifier of the file to be queried
 * @param  st Status buffer to be populated with a response
 * @retval On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
 */
int _fstat_r(struct _reent *ptr, int fd, struct stat *st)
{
	/* TODO remove to save cpu cycles */
	memset (st, 0, sizeof (* st));
	st->st_mode = S_IFCHR;
	return 0;
}


int _stat_r(struct _reent *ptr, const char *name, struct stat *st)
{
	return _fstat_r(ptr, 0, st);
}

/**
 * @brief Indicates whether or not the file/device "is a tty" terminal.
 * @note  Internals of newlib evaluate the return value as an aid in determining how to set
 * buffering mode (line buffer or not) when not specified via setvbuf().
 * @param  ptr newlib re-entrance structure
 * @param  fd File descriptor, identifier of the file to be statused
 * @retval 1 indicates device behaves like a terminal, or 0 when it does not
 */
int _isatty_r(struct _reent *ptr, int fd)
{
	return 1;
}


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
	ptr->_errno = ENOSYS;
	return -1;
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
  * @param  buf: pointer to first character to be written
  * @parm   count: buffer capacity, in bytes
  * @retval -1 = error, zero or positive indicates how many bytes transferred
  */
_ssize_t usb1_read_r (struct _reent *ptr, int file, void *buf, size_t count)
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
  * @note	Redirects to use RTOS method for managing heap
  * @param  size amount of memory to be allocated (as measured by sizeof operator)
  * @retval NULL unable to allocate requested memory, otherwise non-NULL pointer to
  * first element in the allocated memory region.
  */
void *_malloc_r(struct _reent *ptr, size_t size)
{
	return pvPortMalloc(size);
}


/**
 * @brief Allocates a new memory region, then copies existing elements over before freeing
 * old memory region.
 * @note   Redirects to use RTOS method for managing heap
 * @param  new_size amount of memory to be allocated (as measured by sizeof operator)
 * @param  old_ptr Memory region to be freed and returned to memory heap
 * @retval NULL unable to allocate requested memory, otherwise non-NULL pointer to
 * first element in the allocated memory region.
 */
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
  * @note   Compliant stub. Does not use linker command file to allocate memory region,
  * see below for how to allocate memory in source file.
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


/**
 * -------------------------------- Unused stubs ----------------------------------------
 */

#if 0
int _getpid(void)
{
	return 1;
}

int _kill(int pid, int sig)
{
	errno = EINVAL;
	return -1;
}

void _exit (int status)
{
	_kill(status, -1);
	while (1) {}		/* Make sure we hang here */
}


#endif


#if 0

int _lseek(int file, int ptr, int dir)
{
	return 0;
}


int _wait(int *status)
{
	errno = ECHILD;
	return -1;
}

int _unlink(char *name)
{
	errno = ENOENT;
	return -1;
}


int _link(char *old, char *new)
{
	errno = EMLINK;
	return -1;
}

int _fork(void)
{
	errno = EAGAIN;
	return -1;
}

int _execve(char *name, char **argv, char **env)
{
	errno = ENOMEM;
	return -1;
}

#endif

