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



#ifndef PORTABLE_SOCKETIMPL_HPP_
#define PORTABLE_SOCKETIMPL_HPP_

#include <stdint.h>


namespace enl
{

//	typedef Socket_t SocketHandle;

    enum Status
    {
		Ready,        ///< The socket is ready to send / receive data
        NotReady,     ///< The socket is not ready to send / receive data yet
        Done,         ///< The socket has sent / received the data
        Partial,      ///< The TCP socket sent a part of the data
        Disconnected, ///< The TCP socket / WiFi has been disconnected
        Error         ///< An unexpected error happened
    };


    enum class Type
    {
        Tcp, ///< TCP protocol
        Udp  ///< UDP protocol
    };


class SocketImpl
{
public:
	static Status ConnectionStatus(int32_t errorCode);
};


} // namespace enl

#endif /* PORTABLE_SOCKETIMPL_HPP_ */
