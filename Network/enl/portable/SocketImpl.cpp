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

#include "aws_secure_sockets.h"
#include "SocketImpl.hpp"

/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/

/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/
extern "C" BaseType_t WIFI_IsConnected( void );

namespace enl
{

/**
 * @brief  Provides a high level indicator for the overall connection state. The return
 * value is intended to be consistent across different platforms.
 * @retval Current state of the connection
 */
Status SocketImpl::ConnectionStatus(int32_t socketState)
{
	Status retVal = Status::Disconnected;

    if ( WIFI_IsConnected() == pdTRUE )
    {
        switch (socketState)
        {
        case SOCKETS_ERROR_NONE:			retVal = Status::Ready; break;
        case SOCKETS_EWOULDBLOCK:			retVal = Status::NotReady; break;
        case SOCKETS_ECLOSED:				retVal = Status::NotReady; break;
        case SOCKETS_ENOTCONN:				retVal = Status::Disconnected; break;
        case SOCKETS_PERIPHERAL_RESET:		retVal = Status::Error; break;

        /* All other error codes default to the catch all, "Error" */
        default:							retVal = Status::Error; break;
        }
    }

    return retVal;
}


} // namespace enl

