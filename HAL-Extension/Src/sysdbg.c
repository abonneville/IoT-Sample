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


#include "sysdbg.h"
#include "FreeRTOS.h"

#ifdef __cplusplus
using namespace std;
#endif

/**
 * @brief Takes a snapshot of the current heap memory state
 * @param state is the location where the snapshot is stored
 */
void _SysMemCheckpoint(_SysMemState *state)
{
	state->freeHeapSizeNow = xPortGetFreeHeapSize();
	state->freeHeapSizeMin = xPortGetMinimumEverFreeHeapSize();
}

/**
 * @brief Compares two heap memory states to determine if a memory leak has occurred.
 * @note The absolute difference is evaluated
 * @param lhs is the destination where the results are stored
 * @param rhs1 first heap memory state to be evaluated
 * @param rhs2 second heap memory state to be evaluated
 * @retval 1 indicates heap memory leak detected, 0 indicates no leak detected
 */
int _SysMemDifference(_SysMemState *lhs, _SysMemState *rhs1, _SysMemState *rhs2)
{

	// Heap space available now
	if (rhs1->freeHeapSizeNow > rhs2->freeHeapSizeNow) {
		lhs->freeHeapSizeNow = rhs1->freeHeapSizeNow - rhs2->freeHeapSizeNow;
	}
	else {
		lhs->freeHeapSizeNow = rhs2->freeHeapSizeNow - rhs1->freeHeapSizeNow;
	}

	// Heap space all-time low point
	if (rhs1->freeHeapSizeMin < rhs2->freeHeapSizeMin) {
		lhs->freeHeapSizeMin = rhs1->freeHeapSizeMin;
	}
	else {
		lhs->freeHeapSizeMin = rhs2->freeHeapSizeMin;
	}

	return (lhs->freeHeapSizeNow == 0 ? 0 : 1);
}

/**
 * @brief Use to report the results obtained from _SysMemDifference()
 */
void _SysMemDumpStatistics(_SysMemState *state)
{
	printf("Heap - memory leak, size: %u\n", state->freeHeapSizeNow);
	printf("Heap - total as configured, size: %u\n", configTOTAL_HEAP_SIZE);
	printf("Heap - minimum unallocated, size: %u\n", state->freeHeapSizeMin);
	fflush(stdout);
}
