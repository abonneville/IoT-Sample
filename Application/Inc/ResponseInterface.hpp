/*
 * ResponseInterface.hpp
 *
 *  Created on: Mar 27, 2019
 *      Author: Andrew
 */

#ifndef RESPONSEINTERFACE_HPP_
#define RESPONSEINTERFACE_HPP_
#include "thread.hpp"
#include "queue.hpp"

/**
 * @brief Defines a list of response messages that are available. The ResponseInterface
 * will generate the requested message and transmit to an external host.
 */
typedef enum {
	RESPONSE_MSG_AWS_STATUS,
	RESPONSE_MSG_HELP,
	RESPONSE_MSG_INVALID,
	RESPONSE_MSG_PROMPT,
	RESPONSE_MSG_STATUS,
	RESPONSE_MSG_VERSION,
	RESPONSE_MSG_WIFI_STATUS
}ResponseId_t;



/**
 * @brief Implements a persistent thread responsible for generating and transmitting
 * 		  response messages back to an external host.
 */
class ResponseInterface : public cpp_freertos::Thread
{
	public:
		ResponseInterface();
		~ResponseInterface();
		bool putResponse(ResponseId_t responseId, TickType_t Timeout = portMAX_DELAY);

	protected:
		void Run();

	private:
		cpp_freertos::Queue *msgQueue;
};




#endif /* RESPONSEINTERFACE_HPP_ */
