/*
 * syscall.h
 *
 *  Created on: Mar 3, 2019
 *      Author: Andrew
 */

#ifndef SYSCALL_H_
#define SYSCALL_H_

void SYS_CDC_TxCompleteIsr(void);
void SYS_CDC_RxMessageIsr(uint32_t length);



#endif /* SYSCALL_H_ */
