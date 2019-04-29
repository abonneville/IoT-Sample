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

#ifndef THREADCONFIG_HPP_
#define THREADCONFIG_HPP_

#include "FreeRTOS.h"

/**
 * @brief Use when creating/defining a threads priority level. See kernel documentation
 * for how priority determines which thread will run.
 */
typedef enum {
	THREAD_PRIORITY_IDLE = 	0,
	THREAD_PRIORITY_LOWEST = 1,
	THREAD_PRIORITY_BELOW_NORMAL = 2,
	THREAD_PRIORITY_NORMAL = 3,
	THREAD_PRIORITY_ABOVE_NORMAL = 4,
	THREAD_PRIORITY_HIGHEST = 5,
	THREAD_PRIORITY_TIME_CRITICAL =	6
}ThreadPriority_t;

static_assert(configMAX_PRIORITIES == (THREAD_PRIORITY_TIME_CRITICAL + 1),"Mismatch between the number of allowed thread priorities!");

/**
 * @brief Use when creating/defining a threads stack size.
 * @note  For the STM32 port, values here are multiplied by 4 (32-bit value) to set the
 * 		  number of bytes on the stack
 */
typedef uint16_t stack_size_t;
constexpr stack_size_t STACK_SIZE_COMMANDS = (768 / 4);
constexpr stack_size_t STACK_SIZE_RESPONSE = (768 / 4);

#endif /* THREADCONFIG_HPP_ */
