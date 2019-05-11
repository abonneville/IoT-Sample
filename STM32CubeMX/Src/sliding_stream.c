/*
 * sliding_stream.c
 *
 *  Created on: Mar 1, 2019
 *      Author: Andrew
 */
#include "sliding_stream.h"
#include <stdbool.h>

static bool ss_is_buffer_empty(ss_buf_t *buf);
static bool ss_is_buffer_full(ss_buf_t *buf);



/**
  * @brief  Writes the contents of a linear buffer into a ring buffer
  * @param  lhs destination of date, ring buffer
  * @param  rhs source of data, linear buffer
  * @retval SS_OK transfer completed,
  * @retval	SS_FULL partial transfer completed, see rhs->len for balance not transferred
  */
ss_result_t ss_lin_buf_write(ss_buf_t *lhs, ss_lin_buf_t *rhs)
{
	int32_t next;

	// If buffer is not full, then transfer until we run out of data or fill up the buffer
	if (ss_is_buffer_full(lhs)) return SS_FULL;

	while (rhs->len != 0) {
		// Verify one unused space will remain after this write
	    next = lhs->head + 1;  // next is where head will point to after this write.
	    if (next == lhs->maxlen) next = 0; // Wrap to beginning of buffer
	    if (next == lhs->tail) return SS_FULL; // buffer is full

	    // Write a new value into the buffer
	    lhs->data[lhs->head] = rhs->data[0];

	    // Setup for next transfer
	    lhs->head = next;
	    rhs->data++;
	    rhs->len--;
	}

	return SS_OK;
}

/**
  * @brief  Locates the next linear section of data ready to be transferred out
  * @note	Zero copy transfer, contents of lhs updated with section ready to be transferred
  * @param  lhs linear data section to be read from ring buffer
  * @param  rhs source of data, ring buffer
  * @retval SS_OK linear section found, see lhs
  * @retval	SS_EMPTY nothing to be read/transferred
  */
ss_result_t ss_lin_buf_find(ss_lin_buf_t *lhs, ss_buf_t *rhs)
{
	int32_t shadow_head = rhs->head;
	int32_t length;

	if (ss_is_buffer_empty(rhs)) return SS_EMPTY;

	// Set pointer to start of data to be read out
	lhs->data = &rhs->data[rhs->tail];

	// Now determine the length of linear data to be read
	if (rhs->tail < shadow_head)
		length = shadow_head - rhs->tail;
	else
		length = rhs->maxlen - rhs->tail;

	lhs->len = (length < lhs->len) ? length : lhs->len;

	return SS_OK;
}

/**
  * @brief  Deletes specified linear section from ring buffer
  * @note	To be used after calling ss_lin_buf_find
  * @param  lhs linear data section to be deleted from ring buffer
  * @param  rhs source of data, ring buffer
  * @retval SS_OK linear data section removed from ring buffer
  */
ss_result_t ss_lin_buf_delete(ss_lin_buf_t *lhs, ss_buf_t *rhs)
{
	rhs->tail += lhs->len;
    if(rhs->tail == rhs->maxlen) rhs->tail = 0; // wrap to beginning of buffer
    return SS_OK;
}



static bool ss_is_buffer_empty(ss_buf_t *buf)
{
	return (buf->head == buf->tail);
}

static bool ss_is_buffer_full(ss_buf_t *buf)
{
	return (buf->head + 1 == buf->tail);
}


// These methods are untested, but potential for future development
#if 0
int32_t ss_buf_write(ss_buf_t *lhs, uint8_t data)
{
	int32_t next;

    next = lhs->head + 1;  // next is where head will point to after this write.
    if (next >= lhs->maxlen)
        next = 0;

    if (next == lhs->tail)  // if the head + 1 == tail, circular buffer is full
        return -1;

    lhs->data[lhs->head] = data;  // Load data and then move
    lhs->head = next;             // head to next data offset.
    return 0;  // return success to indicate successful push.
}


int32_t ss_buf_read(ss_buf_t *rhs, uint8_t *data)
{
	int32_t next;

    if (rhs->head == rhs->tail)  // if the head == tail, we don't have any data
        return -1;

    next = rhs->tail + 1;  // next is where tail will point to after this read.
    if(next >= rhs->maxlen)
        next = 0;

    *data = rhs->data[rhs->tail];  // Read data and then move
    rhs->tail = next;              // tail to next offset.
    return 0;  // return success to indicate successful push.
}


int32_t ss_lin_buf_read(ss_lin_buf_t *lhs, ss_buf_t *rhs)
{
	int32_t next;

	// If buffer is not empty, then transfer until we run out of data or fill up the destination
	if (ss_is_buffer_empty(rhs)) return -1;

	while (lhs->len != 0) {
	    lhs->data = &rhs->data[rhs->tail];

	    // Setup for next transfer
	    lhs->data++;
	    lhs->len--;
	    rhs->tail++;
	    if(rhs->tail == rhs->maxlen) next = 0; // wrap to beginning of buffer
	    if (rhs->tail == rhs->head) return -1; // buffer is empty
	}
	return 0;
}
#endif


