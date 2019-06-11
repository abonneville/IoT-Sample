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

#include "aws_secure_sockets.h"

/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/

/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/



namespace enl
{


/**
 * Pimpl idiom is being used to keep vendor/driver types out of the header, and avoid
 * header dependency.
 */
class WiFiClient::impl
{
public:
	impl() :
		_type(Type::Tcp),
		_socket(SOCKETS_INVALID_SOCKET),
		socketState(SOCKETS_ENOTCONN),
		transportSettings {}
	{};

	impl(Type type) :
		_type(type),
		_socket(SOCKETS_INVALID_SOCKET),
		socketState(SOCKETS_ENOTCONN),
		transportSettings {}
	{};

	impl(intptr_t sock) :
		_type(Type::Tcp),
		_socket( (Socket_t)sock ),
		socketState(SOCKETS_ENOTCONN),
		transportSettings {}
	{};

	Type _type;
	Socket_t _socket;
	int32_t socketState;
	ES_WIFI_Transport_t transportSettings;
};


WiFiClient::~WiFiClient()
{
}


/**
 * @brief  Constructor
 * @param  None
 * @retval None
 */
WiFiClient::WiFiClient() :
		pimpl{ std::make_unique<impl>() }
{
}


/**
 * @brief  Constructor
 * @param  type sets protocol for communicating
 * @retval None
 */
WiFiClient::WiFiClient(Type type) :
		pimpl{ std::make_unique<impl>(type) }
{
}


/**
 * @brief  Constructor
 * @note   Used by WiFiServer when new client connects
 * @param  sock: socket to use
 * @retval None
 */
WiFiClient::WiFiClient(intptr_t sock) :
		pimpl{ std::make_unique<impl>(sock) }
{
}


/**
* @brief  Connect to the IP address and port specified in the constructor
* @param  ip : IP to which to send the packet
* @param  port : port to which to send the packet
* @retval True if the connection succeeds, false if not.
*/
bool WiFiClient::connect(IPAddress & ip, uint16_t port)
{

	if (pimpl->_socket == SOCKETS_INVALID_SOCKET)
	{
		if ( pimpl->_type == Type::Tcp )
		{
			pimpl->_socket = SOCKETS_Socket( SOCKETS_AF_INET, SOCKETS_SOCK_STREAM, SOCKETS_IPPROTO_TCP );
		}
		else
		{
			pimpl->_socket = SOCKETS_Socket( SOCKETS_AF_INET, SOCKETS_SOCK_DGRAM, SOCKETS_IPPROTO_UDP );
		}
	}
/**
 * TODO: add unit test and implement fix for when ip = 0.0.0.0, treat as invalid and do not connect
 */
	if (pimpl->_socket != SOCKETS_INVALID_SOCKET)
	{
		// set connection parameter and start client
		SocketsSockaddr_t hostAddress = { 0 };
		hostAddress.usPort = SOCKETS_htons(port);
		hostAddress.ulAddress = ip;
		hostAddress.ucSocketDomain = SOCKETS_AF_INET;
		pimpl->socketState = SOCKETS_Connect( pimpl->_socket, &hostAddress, sizeof(hostAddress) );
		if ( pimpl->socketState == SOCKETS_ERROR_NONE )
		{
			/* Valid connection, capture settings */
			SOCKETS_GetTransportSettings(pimpl->_socket, &pimpl->transportSettings);
		}
	}

  return ( pimpl->socketState == SOCKETS_ERROR_NONE);
}

/**
 * @brief  Connect to the domain name specified in the constructor
 * @param  host : host to which to send the packet
 * @param  port : port to which to send the packet
 * @retval True if the connection succeeds, false if not.
 */
bool WiFiClient::connect(const char *host, uint16_t port)
{
	IPAddress ip; // IP address of the host

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
	int32_t SentLen = SOCKETS_Send(pimpl->_socket, buf, size, 0);

	if ( SentLen < 0 )
	{
		pimpl->socketState = SentLen;
		return 0;
	}

	pimpl->socketState = SOCKETS_ERROR_NONE;
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

	int32_t RecLen = SOCKETS_Recv( pimpl->_socket, buf, size, 0);

	if ( RecLen < 0)
	{
		pimpl->socketState = RecLen;
		return 0;
	}

	pimpl->socketState = SOCKETS_ERROR_NONE;
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
	SOCKETS_Close( pimpl->_socket );

	pimpl->socketState = SOCKETS_ENOTCONN;
	pimpl->_socket = SOCKETS_INVALID_SOCKET;
	std::memset( &pimpl->transportSettings, 0, sizeof( pimpl->transportSettings ) );
}

/**
 * @brief  Whether or not the client is connected.
   @param  None
 * @retval true when connected, false when not connected
 */
bool WiFiClient::connected()
{
	return ( SocketImpl::ConnectionStatus( pimpl->socketState ) == Status::Ready );
}

/**
 * @brief  Get connection state
   @param  None
 * @retval Status, high level indicator of connection status
 */
Status WiFiClient::status()
{
	return SocketImpl::ConnectionStatus( pimpl->socketState );
}

/**
  * @brief  Evaluates if the object contains a valid socket handle
  * @param  None
  * @retval None.
  */
WiFiClient::operator bool()
{
	return pimpl->_socket != SOCKETS_INVALID_SOCKET;
}


/**
 * @brief  Gets the IP address of the remote connection.
 * @note   Intended to be used when "client" object is obtained from a server
 * @retval The host IP address currently connected to.
 */
IPAddress WiFiClient::remoteIP()
{
	IPAddress ip { pimpl->transportSettings.Remote_IP_Addr };
	return ip;
}


/**
 * @brief  Gets the port of the remote connection.
 * @note   Intended to be used when "client" object is obtained from a server
 * @retval The host port number currently connected to.
 */
uint16_t WiFiClient::remotePort()
{

	return pimpl->transportSettings.Remote_Port;
}

} /* namespace enl */
