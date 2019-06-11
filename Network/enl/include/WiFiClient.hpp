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



#ifndef INCLUDE_WIFICLIENT_HPP_
#define INCLUDE_WIFICLIENT_HPP_

#include "NetworkAddress.hpp"
#include "SocketImpl.hpp"
#include <stdint.h>
#include <memory>

namespace enl
{
class WiFiClient
{
public:
	WiFiClient();
	WiFiClient(Type type);
	WiFiClient(intptr_t sock);
	~WiFiClient();
	// TODO WiFiClient(const WiFiClient & ) = delete;

	Status status();
	bool connect(IPAddress & ip, uint16_t port);
	bool connect(const char *host, uint16_t port);
	size_t write(uint8_t);
	size_t write(const uint8_t *buf, size_t size);
	size_t write(const char    *buf, size_t size)
		{
			return write( (const uint8_t *)buf, size );
		};
	int32_t available();
	int read();
	size_t read(uint8_t *buf, size_t size);
	size_t read(char    *buf, size_t size)
    {
      return read((uint8_t *)buf, size);
    };

	int peek();
	void flush();
	void stop();
	bool connected();
	operator bool();

	IPAddress remoteIP();
	uint16_t remotePort();

protected:

private:
	class impl;
	std::unique_ptr<impl> pimpl;

};

} /* namespace enl */


#endif /* INCLUDE_WIFICLIENT_HPP_ */
