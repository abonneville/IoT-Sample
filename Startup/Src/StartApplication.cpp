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

#include <StartApplication.hpp>
#include "CommandInterface.hpp"
#include "ResponseInterface.hpp"


/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/
static ResponseInterface rspThread;
static CommandInterface cmdThread = CommandInterface(rspThread);

/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/


/**
  * @brief  The application entry point to C++
  * @note	This method is intended to be called after platform initialization is completed
  * 		in main()
  */
void StartApplication(void)
{
	/* Note: When the RTOS scheduler is started, the main stack pointer (MSP) is reset,
	 * effectively wiping out all stack/local main() variables and objects. Do not declare
	 * any C/C++ threads here. If objects need to be declared here, then change the code
	 * within prvPortStartFirstTask() to retain the MSP value.
	 */
	cpp_freertos::Thread::StartScheduler();

	/* Infinite loop */
	while (1)
	{
	}
}



