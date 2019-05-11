/*
 * sliding_stream.h
 *
 *  Created on: Mar 2, 2019
 *      Author: Andrew
 */


#ifndef SLIDING_STREAM_H_
#define SLIDING_STREAM_H_

#include <stdint.h>

typedef enum {
	SS_OK,
	SS_FULL,
	SS_EMPTY
} ss_result_t;

typedef struct {
    uint8_t * const data; // pointer to compile time buffer
    int32_t tail; // start of data
    int32_t head; // last data + 1
    const int32_t maxlen; // size of
} ss_buf_t;

#define SS_BUF_DEF(x,y)                \
    uint8_t x##_data_space[y + 1];            \
    ss_buf_t x = {                        \
        .data = x##_data_space,         \
        .tail = 0,                        \
        .head = 0,                        \
        .maxlen = y + 1                    \
    }

typedef struct {
    uint8_t *data;
    int32_t len;
} ss_lin_buf_t;


ss_result_t ss_lin_buf_write(ss_buf_t *lhs, ss_lin_buf_t *rhs);
ss_result_t ss_lin_buf_find(ss_lin_buf_t *lhs, ss_buf_t *rhs);
ss_result_t ss_lin_buf_delete(ss_lin_buf_t *lhs, ss_buf_t *rhs);



#endif /* SLIDING_STREAM_H_ */
