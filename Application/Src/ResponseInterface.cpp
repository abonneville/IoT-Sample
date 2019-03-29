/*
 * ResponseInterface.cpp
 *
 *  Created on: Mar 27, 2019
 *      Author: Andrew
 */

#include <string>
#include <cstdio>

#include "ticks.hpp"
#include "ThreadInterface.hpp"
#include "ResponseInterface.hpp"

using namespace cpp_freertos;
using namespace std;

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/


/**
 * @brief Creates a thread and a message queue to handle requests to generate response
 * messages.
 */
ResponseInterface::ResponseInterface()
       : Thread("ResponseInterface", STACK_SIZE_RESPONSE, THREAD_PRIORITY_NORMAL)
{
	constexpr static UBaseType_t itemSize = sizeof(ResponseId_t);
	constexpr static UBaseType_t maxItems = 5;
	msgQueue = new Queue(maxItems, itemSize);

	Start();
}

ResponseInterface::~ResponseInterface()
{
	delete msgQueue;
}

/**
 *  Add a response item to the back of the queue.
 *
 *  @param responseId is the response item you are adding.
 *  @param Timeout is how long to wait to add the item to the queue if
 *         the queue is currently full.
 *  @return true if the item was added, false if it was not.
 */
bool ResponseInterface::putResponse(ResponseId_t responseId, TickType_t Timeout)
{
	return msgQueue->Enqueue(&responseId, Timeout);
}

/**
 * @brief Implements the persistent loop for thread execution.
 */
void ResponseInterface::Run()
{
	ResponseId_t msgId = RESPONSE_MSG_INVALID;
	while (true) {

		// Wait until a request is made to generate a response message
		msgQueue->Dequeue(&msgId);

		printf("%lu - Response Interface - %i\n", Ticks::GetTicks(), msgId);
		fflush(stdout);


		/*
		// USB enumeration between host and device can/will be delayed on startup, so cout buffer will need
		// special handling to establish data stream
		if (cout.fail()) {
			cout.clear();
		}
		*/

	}

}
