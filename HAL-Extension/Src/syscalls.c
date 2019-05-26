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
#include "device.h"
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


typedef struct {
	int flags;
}DeviceControl_t;

DeviceControl_t DeviceHandle[FOPEN_MAX] = {};

typedef enum {
	fdInvalid = -1,
	std_in = STDIN_FILENO,
	std_out = STDOUT_FILENO,
	std_err = STDERR_FILENO,
	storage
} FileDescriptor_t;


const Device_t Device = {.std_in = "std_in", .std_out = "std_out", .std_err = "std_err", .storage = "storage"};


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


static const DeviceOperations_t dvStorage = {
		Device.storage, sizeof(Device.storage),
		storage_open_r,
		storage_close_r,
		storage_write_r,
		storage_read_r
};


/**
 * The order in which devices are added to the DeviceList, corresponds to its "file descriptor" (fd)
 * value used by the newlib library. Newlib pre-defines the first three fd, additional files/devices
 * can be added up to FOPEN_MAX.
 */
static const DeviceOperations_t *DeviceList[] = {
		&dvStdin, /* fd = [0], STDIN_FILENO */
		&dvStdout, /* fd = [1], STDOUT_FILENO */
		&dvStderr, /* fd = [2], STDERR_FILENO */
		&dvStorage,

		0 /* terminates the list */
};

static_assert((sizeof(DeviceList) / sizeof(intptr_t)) - 1  <= FOPEN_MAX, "Error: device list exceeds newlibs internal list for open streams");


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

	case storage:
//TODO		setvbuf(handle, NULL, _IONBF, 0);
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
	   int ret = -1;

	   /* Set file descriptor based on provided name */
	   do {
	      if( memcmp( name, DeviceList[index]->name, DeviceList[index]->nameSize ) == 0 ) {
	         fd = index;
	         break;
	      }
	   } while( DeviceList[index++] );

	   if( fd != -1 ) {
		   vTaskSuspendAll();
		   /* Found a match, attempt to open device */
		   if (DeviceHandle[fd].flags == 0) {
			   DeviceHandle[fd].flags = 1;
			   xTaskResumeAll();

			   ret = DeviceList[fd]->_open_r( ptr, fd, flags, mode );

			   if (ret < 0) {
				   DeviceHandle[fd].flags = 0;
			   }
			   return ret;
		   }

		   xTaskResumeAll();
		   ptr->_errno = EACCES;
	   }
	   else {
		   ptr->_errno = ENODEV;
	   }

	   return -1;
}

int _close_r(struct _reent *ptr, int fd)
{
	vTaskSuspendAll();
	if (DeviceHandle[fd].flags == 1) {
	   DeviceHandle[fd].flags = 0;
	   xTaskResumeAll();

	   return DeviceList[fd]->_close_r( ptr, fd );
	}

    xTaskResumeAll();
	return 0;
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
	memset (st, 0, sizeof (* st));
	st->st_mode = S_IFCHR;
	if (fd == storage) {
		st->st_mode = S_IFREG;
		st->st_size = 2048;
	}
	return 0;
}


int _stat_r(struct _reent *ptr, const char *name, struct stat *st)
{
	return _fstat_r(ptr, 0, st);
}


/**
 * @brief  Repositions the file offset of the open file description associated with the file
 * 		   descriptor fd to the argument offset according to the directive whence
 * @ note  lseek() allows the file offset to be set beyond the end of the file
 *		   (but this does not change the size of the file).  If data is later written at
 *		   this point, subsequent reads of the data in the gap (a "hole") return null
 *		   bytes ('\0') until data is actually written into the gap.
 *
 * @param  offset Number of bytes to be applied based on the "whence" directive.
 * @param  ptr newlib re-entrance structure
 * @param  fd File descriptor, identifier of the file to be queried
 * @param  whence Either/or:
 *         SEEK_SET - The file offset is set to offset bytes.
 *         SEEK_CUR - The file offset is set to its current location plus offset bytes.
 *         SEEK_END - The file offset is set to the size of the file plus offset bytes.
 *
 * @retval On success, returns the resulting offset location as measured in bytes from the
 * 		beginning of the file. On error, -1 is returned, and errno is set appropriately.
 */
_off_t _lseek_r(struct _reent *ptr, int fd, _off_t offset, int whence)
{
	ptr->_errno = EINVAL;
	return -1;
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
  * @brief  Standard C method for accessing process time, or run time.
  * @note   time.h defines CLOCKS_PER_SEC to be 100, 10mS period. Not accurate, time reported
  * is actually 1mS period. Consider using configTICK_RATE_HZ instead.
  * @param  buf process times. Single processor, so only utime (user time) is set
  * @retval 0 on success
  */
clock_t _times_r(struct _reent *ptr, struct tms *buf)
{
	clock_t sysTickCount = xTaskGetTickCount();
	buf->tms_utime = sysTickCount;
	buf->tms_stime = 0;
	buf->tms_cutime = 0;
	buf->tms_cstime = 0;

	return 0;
}



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

