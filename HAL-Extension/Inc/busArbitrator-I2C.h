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



#ifndef BUSARBITRATOR_I2C_H_
#define BUSARBITRATOR_I2C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stddef.h"

/* Defines a list of all the I2C busses that are managed by this interface.
 *
 *  1.) Insert a new field here for each supported bus, order is not important.
 *  2.) Add necessary initialization in BA_I2C_init(...)
 */
typedef enum
{
	I2C2_Bus,

	/* Insert new bus above this definition */
	NumberOfI2cBusses
}BA_I2C_descriptor_t;


typedef struct
{
	/* outbound message buffer to be sent to device */
	uint8_t *outbound;
	size_t outSize;

	/* inbound message buffer to hold data received from device */
	uint8_t *inbound;
	size_t inSize;
}BA_I2C_buffer_t;

void BA_I2C_init( BA_I2C_descriptor_t bus );
void BA_I2C_read( BA_I2C_descriptor_t bus, uint16_t deviceAddr, BA_I2C_buffer_t *buffer);
void BA_I2C_write( BA_I2C_descriptor_t bus, uint16_t deviceAddr, BA_I2C_buffer_t *buffer);


#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* BUSARBITRATOR_I2C_H_ */
