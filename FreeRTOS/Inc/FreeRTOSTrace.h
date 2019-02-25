/*
 * FreeRTOSTrace.h
 *
 *  Created on: Feb 9, 2019
 *      Author: Andrew
 */

#ifndef FREERTOSTRACE_H_
#define FREERTOSTRACE_H_

/* Definitions needed when configGENERATE_RUN_TIME_STATS is on */
#if (configGENERATE_RUN_TIME_STATS > 0)
// HAL_GetTick is provided by the BSP maintained in in a separate build project. To avoid
// coupling the projects and invoking nested include paths, a prototype is provided via "extern"
// to be resolved during final link.
extern uint32_t HAL_GetTick(void);
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS 				HAL_GetTick // do nothing, already setup by HAL
#define portGET_RUN_TIME_COUNTER_VALUE                      HAL_GetTick

#endif

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    error_handler(1, _ERR_Stack_Overflow);
}
#endif


#if (configCHECK_FOR_STACK_OVERFLOW > 0)
volatile uint32_t r0;
volatile uint32_t r1;
volatile uint32_t r2;
volatile uint32_t r3;
volatile uint32_t r12;
volatile uint32_t lr; /* Link register. */
volatile uint32_t pc; /* Program counter. */
volatile uint32_t psr;/* Program status register. */

void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimising them
away as the variables never actually get used.  If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */
    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* When the following line is hit, the variables contain the register values. */
    for( ;; );
}
#endif


#endif /* FREERTOSTRACE_H_ */
