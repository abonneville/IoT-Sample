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

#include <reent.h> /* Reentrant versions of system calls.  */
#include <stddef.h>
#include <stdint.h>

#ifndef DEVICE_H_
#define DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

int usb1_open_r(struct _reent *ptr, int fd, int flags, int mode);
int usb1_close_r(struct _reent *ptr, int fd);
_ssize_t usb1_write_r(struct _reent *ptr, int fd, const void *buf, size_t len);
_ssize_t usb1_read_r (struct _reent *ptr, int fd, void *buf, size_t len);

void SYS_CDC_TxCompleteIsr(void);
void SYS_CDC_RxMessageIsr(uint32_t length);


int storage_open_r(struct _reent *ptr, int fd, int flags, int mode);
int storage_close_r(struct _reent *ptr, int fd);
_ssize_t storage_write_r(struct _reent *ptr, int fd, const void *buf, size_t len);
_ssize_t storage_read_r (struct _reent *ptr, int fd, void *buf, size_t len);

uint32_t crc_mpeg2(uint8_t *first, uint8_t *last);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DEVICE_H_ */
