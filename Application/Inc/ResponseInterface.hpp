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

#ifndef RESPONSEINTERFACE_HPP_
#define RESPONSEINTERFACE_HPP_
#include "thread.hpp"
#include "queue.hpp"

class UserConfig;

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
		ResponseInterface(cpp_freertos::Queue &, UserConfig &);
		~ResponseInterface();

	protected:
		void Run();

		void AwsStatusHandler();
		void WifiStatusHandler();

	private:
		cpp_freertos::Queue &msgHandle;
		UserConfig &userConfigHandle;
};




#endif /* RESPONSEINTERFACE_HPP_ */
