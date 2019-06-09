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

#include "WiFiClient.hpp"

/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/

/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/



namespace enl
{


/**
 * @brief  Constructor
 * @param  None
 * @retval None
 */
WiFiClient::WiFiClient() :
		_type(Type::Tcp),
		_socket(SOCKETS_INVALID_SOCKET),
		socketState(SOCKETS_ENOTCONN)
{
}


/**
 * @brief  Constructor
 * @param  type sets protocol for communicating
 * @retval None
 */
WiFiClient::WiFiClient(Type type) :
		_type(type),
		_socket(SOCKETS_INVALID_SOCKET),
		socketState(SOCKETS_ENOTCONN)
{
}


/**
 * @brief  Constructor
 * @note   Used by WiFiServer when new client connects
 * @param  sock: socket to use
 * @retval None
 */
WiFiClient::WiFiClient(Socket_t sock) :
		_type(Type::Tcp),
		_socket(sock),
		socketState(SOCKETS_ENOTCONN)
{
}

/**
* @brief  Connect to the IP address and port specified in the constructor
* @param  ip : IP to which to send the packet
* @param  port : port to which to send the packet
* @retval True if the connection succeeds, false if not.
*/
bool WiFiClient::connect(uint32_t ip, uint16_t port)
{

	if (_socket == SOCKETS_INVALID_SOCKET)
	{
		if ( _type == Type::Tcp )
		{
			_socket = SOCKETS_Socket( SOCKETS_AF_INET, SOCKETS_SOCK_STREAM, SOCKETS_IPPROTO_TCP );
		}
		else
		{
			_socket = SOCKETS_Socket( SOCKETS_AF_INET, SOCKETS_SOCK_DGRAM, SOCKETS_IPPROTO_UDP );
		}
	}
/**
 * TODO: add unit test and implement fix for when ip = 0.0.0.0, treat as invalid and do not connect
 */
	if (_socket != SOCKETS_INVALID_SOCKET)
	{
		// set connection parameter and start client
		SocketsSockaddr_t hostAddress = { 0 };
		hostAddress.usPort = SOCKETS_htons(port);
		hostAddress.ulAddress = ip;
		hostAddress.ucSocketDomain = SOCKETS_AF_INET;
		socketState = SOCKETS_Connect( _socket, &hostAddress, sizeof(hostAddress) );
	}

  return ( socketState == SOCKETS_ERROR_NONE);
}

/**
 * @brief  Connect to the domain name specified in the constructor
 * @param  host : host to which to send the packet
 * @param  port : port to which to send the packet
 * @retval True if the connection succeeds, false if not.
 */
bool WiFiClient::connect(const char *host, uint16_t port)
{
	uint32_t ip; // IP address of the host

	ip = SOCKETS_GetHostByName( host );
	return connect(ip, port);
}

/**
 * @brief  Write size bytes from buffer into the packet
 * @param  buf : data to write
 * @param  size : size of data to write
 * @retval Returns the number of bytes written
 */
size_t WiFiClient::write(const uint8_t *buf, size_t size)
{
	int32_t SentLen = SOCKETS_Send(_socket, buf, size, 0);

	if ( SentLen < 0 )
	{
		socketState = SentLen;
		return 0;
	}

	socketState = SOCKETS_ERROR_NONE;
	return SentLen;
}

/**
 * @brief  Write a single byte into the packet
 * @param  b : data to write
 * @retval Returns the number of bytes written
 */
size_t WiFiClient::write(uint8_t b)
{
	return write(&b, 1);
}

/**
 * @brief  Indicates if there is one or more bytes available to be read
 * @param  None
 * @retval 1 if data available, 0 if not.
 */
int32_t WiFiClient::available()
{
	/* Device does not support a 0-byte query to determine if data is pending. Therefore,
	 * an attempt to read 1 or more bytes would be required to detect if data is pending.
	 *
	 * Local buffering a single byte is not warranted at this time.
	 */
	return 0;
}

/**
 * @brief  Read a single byte from the current packet
 * @param  None
 * @retval The next byte (or character), or -1 if none is available.
 */
int WiFiClient::read()
{
	uint8_t buf = '\0'; // data received
	return ( read( &buf, 1 ) == 0 ? -1 : buf) ;
}

/**
 * @brief  Read up to size bytes from the current packet and place them into buffer
 * @param  buffer : Where to place read data
 * @param  size : length of data to read
 * @retval Returns the number of bytes read, or 0 if none are available
 */
size_t WiFiClient::read(uint8_t *buf, size_t size)
{

	int32_t RecLen = SOCKETS_Recv( _socket, buf, size, 0);

	if ( RecLen < 0)
	{
		socketState = RecLen;
		return 0;
	}

	socketState = SOCKETS_ERROR_NONE;
	return RecLen;
}

/**
 * @brief  Return the next byte from the current packet without moving on to the next byte
 * @param  None
 * @retval next byte from the current packet without moving on to the next byte
 * @Note Function not supported , always return 0.
 */
int WiFiClient::peek()
{
	/***************************************************************************/
	/*                               NOT SUPPORTED                             */
	/* This functionality doesn't exist in the current device.                 */
	/***************************************************************************/
	return 0;
}

/**
 * @brief  Finish reading the current packet
 * @param  None
 * @retval None
 * @Note Function not supported
 */
void WiFiClient::flush()
{
  /***************************************************************************/
  /*                               NOT SUPPORTED                             */
  /* Not supported in previous device, still not supported                   */
  /***************************************************************************/
}

/**
 * @brief  Close the client connection
 * @param  None
 * @retval None
 */
void WiFiClient::stop()
{
	SOCKETS_Close( _socket );

	socketState = SOCKETS_ENOTCONN;
	_socket = SOCKETS_INVALID_SOCKET;
}

/**
 * @brief  Whether or not the client is connected.
   @param  None
 * @retval true when connected, false when not connected
 */
bool WiFiClient::connected()
{
	return ( SocketImpl::ConnectionStatus( socketState ) == Status::Ready );
}

/**
 * @brief  Get connection state
   @param  None
 * @retval Status, high level indicator of connection status
 */
Status WiFiClient::status()
{
	return SocketImpl::ConnectionStatus( socketState );
}

/**
  * @brief  Evaluates if the object contains a valid socket handle
  * @param  None
  * @retval None.
  */
WiFiClient::operator bool()
{
	return _socket != SOCKETS_INVALID_SOCKET;
}


/**
 * @brief  Gets the IP address of the remote connection.
 * @note   Intended to be used when "client" object is obtained from a server
 * @retval The IP address of the host currently connected to.
 */
uint32_t WiFiClient::remoteIP()
{
	uint32_t retVal = 0;
	uint8_t address[4] = {};
	uint16_t port = 0;
	SOCKETS_GetRemoteData(_socket, address, &port);

	retVal  = address[0] << 24;
	retVal |= address[1] << 16;
	retVal |= address[2] <<  8;
	retVal |= address[3];
	return retVal;
}


/**
 * @brief  Gets the port of the remote connection.
 * @note   Intended to be used when "client" object is obtained from a server
 * @retval The port of the host currently connected to.
 */
uint16_t WiFiClient::remotePort()
{

	return 0;
}

} /* namespace enl */
