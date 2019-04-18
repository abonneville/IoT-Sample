/*
 * ThreadInterface.hpp
 *
 *  Created on: Mar 27, 2019
 *      Author: Andrew
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
