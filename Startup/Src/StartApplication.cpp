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

#include "StartApplication.hpp"
#include "UserConfig.hpp"
#include "CommandInterface.hpp"
#include "ResponseInterface.hpp"
#include "CloudInterface.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include "aws_logging_task.h"

#ifdef __cplusplus
}
#endif /* extern "C" */



/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/
#define mainLOGGING_TASK_PRIORITY           (UBaseType_t)( configMAX_PRIORITIES - 1 )
#define mainLOGGING_TASK_STACK_SIZE         ( configMINIMAL_STACK_SIZE * 2 )
#define mainLOGGING_MESSAGE_QUEUE_LENGTH    (UBaseType_t)( 15 )

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/

/*
 * All thread objects are being instantiated and linked via handle here, to avoid undefined
 * initialization if they were instantiated in different translation units (files). As this
 * project grows, this approach may have to change and use a singleton and/or factory design
 * pattern for distributing initialized object handles.
 */
extern "C" {

UserConfig userConfig;
}

constexpr static UBaseType_t itemSize = sizeof(ResponseId_t);
constexpr static UBaseType_t maxItems = 5;
static cpp_freertos::Queue msgQueue(maxItems, itemSize, "msgQueue");

static ResponseInterface rspThread(msgQueue, userConfig);
static CommandInterface cmdThread(msgQueue, userConfig);

static CloudInterface cloudThread(userConfig);

/* Function prototypes -----------------------------------------------*/


/* External functions ------------------------------------------------*/


/**
  * @brief  The application entry point to C++. Initializes middleware, and launches
  * 		kernel/application layer.
  * @note	This method is intended to be called after platform/HAL initialization is completed
  * 		in main()
  */
void StartApplication(void)
{
	/* Note: When the RTOS scheduler is started, the main stack pointer (MSP) is reset,
	 * effectively wiping out all stack/local main() variables and objects. Do not declare
	 * any C/C++ threads here. If objects need to be declared here, then change the code
	 * within prvPortStartFirstTask() to retain the MSP value.
	 */

	xLoggingTaskInitialize( mainLOGGING_TASK_STACK_SIZE,
	                        mainLOGGING_TASK_PRIORITY,
	                        mainLOGGING_MESSAGE_QUEUE_LENGTH );


	cpp_freertos::Thread::StartScheduler();

	/* Infinite loop */
	while (1)
	{
	}
}
