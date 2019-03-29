/*
 * CommandInterface.cpp
 *
 *  Created on: Mar 27, 2019
 *      Author: Andrew
 */
#include <cstdio>
#include <algorithm>

#include "ticks.hpp"
#include "ThreadInterface.hpp"
#include "CommandInterface.hpp"
#include "ResponseInterface.hpp"

using namespace cpp_freertos;

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
bool invalidChar (char c);

/**
 * @brief Binds the CommandInterface to the requested ResponseInterface, then creates
 * the thread. If the scheduler is already running, then then thread will begin
 * @param handle identifies which ResponseInterface will handle command response messaging
 */
CommandInterface::CommandInterface(ResponseInterface &handle)
	: Thread("CommandInterface", STACK_SIZE_COMMANDS, THREAD_PRIORITY_ABOVE_NORMAL),
	  responseHandle(handle)
{
	Start();
}

/**
 * @brief Implements the persistent loop for thread execution.
 */
void CommandInterface::Run()
{

    while (true) {

    	// Wait for a user command
    	std::fgets(commandLineBuffer.begin(), commandLineBuffer.size(), stdin);

    	// Remove all non-printable character(s)
    	std::remove_if(commandLineBuffer.begin(), commandLineBuffer.end(), invalidChar);


    	// Parse and execute command handler for further processing

    	// Request response message be sent to host
    	responseHandle.putResponse(RESPONSE_MSG_STATUS, (TickType_t)10);

    	std::printf("%lu - %s", Ticks::GetTicks(), commandLineBuffer.begin());
    	std::fflush(stdout);

    	// Log any and all error states for future report back to host

/*

    	// USB enumeration between host and device can/will be delayed on startup, so cout buffer will need
    	// special handling to establish data stream
    	if (cout.fail()) {
    		cout.clear();
    	}
*/

    }
}



/**
  * @brief  Indicates if character is outside valid range of characters
  * @param  c character to be evaluated
  * @retval true when an invalid character is detected, false when character is within valid range
  */
bool invalidChar (char c)
{
	// Valid range is all printable characters, or the null terminator
    return !((c > 0x1F && c < 0x7F) || c == 0x00);
}
