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

#include "CppUTest/TestHarness.h"
#include "FreeRTOS.h"
#include "aws_wifi.h"

#include "TestConfig.hpp"
#include "thread.hpp"

#include "WiFiClient.hpp"


/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/
extern uint8_t bigData[ BIG_DATA_SIZE ];
static enl::WiFiClient client;

static constexpr char password[] = "sunshine";
static constexpr char ssid[] = "Dev-2G";
static constexpr char server[] = "www.google.com";

/* Function prototypes -----------------------------------------------*/
static void httpRequest( void );

/* External functions ------------------------------------------------*/

TEST_GROUP(wifi) {};

TEST_GROUP(WiFiClient) {
	void setup()
	{
		/* Connect to the network via WiFi */
		WIFINetworkParams_t networkParms = {};
		networkParms.pcPassword = password;
		networkParms.ucPasswordLength = sizeof(password) - 1;
		networkParms.pcSSID = ssid;
		networkParms.ucSSIDLength = sizeof(ssid) - 1;
		networkParms.xSecurity = eWiFiSecurityWPA2;

		/* Initializes WiFi API, must be called first */
		CHECK( eWiFiSuccess == WIFI_On() );

		CHECK( eWiFiSuccess == WIFI_ConnectAP(&networkParms) );
		CHECK( 1 == WIFI_IsConnected() );

		CHECK( enl::Status::Disconnected == client.status() );
		CHECK( false == client.connected() );
		CHECK( true == client.connect(server, 80) );
		CHECK( true == client.connected() );
		CHECK( enl::Status::Ready == client.status() );
		CHECK( true == client );

	}

	void teardown()
	{
		/* Note: if you add a "test" here and it fails, the tear down process is aborted
		 */
		client.stop();
		WIFI_Disconnect();

		/* Teardown complete, verify client is disconnected...
		 */
		CHECK( false == client.connected() );
		CHECK( enl::Status::Disconnected == client.status() );
		CHECK( false == client );

	}
};



/**
 * @brief  Makes an HTTP connection to the server
 */
static void httpRequest( void )
{
	char msg[100];
	int length = 0;

	// Make a HTTP request:
	length = snprintf(msg, 100, "GET /search?q=arduino HTTP/1.1\r\n");
	client.write(msg, length);
	length = snprintf(msg, 100, "Host: www.google.com\r\n");
	client.write(msg, length);
	length = snprintf(msg, 100, "Connection: close\r\n");
	client.write(msg, length);
	length = snprintf(msg, 100, "\r\n");
	client.write(msg, length);
}







TEST(wifi, Connect)
{
	/* Connect to the network via WiFi */
	WIFINetworkParams_t networkParms = {};
	networkParms.pcPassword = password;
	networkParms.ucPasswordLength = sizeof(password) - 1;
	networkParms.pcSSID = ssid;
	networkParms.ucSSIDLength = sizeof(ssid) - 1;
	networkParms.xSecurity = eWiFiSecurityWPA2;

	/* Initializes WiFi API, must be called first */
	CHECK( eWiFiSuccess == WIFI_On() );
	CHECK( eWiFiSuccess == WIFI_On() ); /* Just repeating to verify behavior */

	CHECK( 0 == WIFI_IsConnected() );
	CHECK( eWiFiSuccess == WIFI_ConnectAP(&networkParms) );
	CHECK( 1 == WIFI_IsConnected() );

	/* Ping LAN device to confirm network access */
	uint8_t lanAddress[4] = { 192, 168, 1, 1};
	static constexpr uint16_t count = 3;
	static constexpr uint32_t interval_mS = 10;
	CHECK( eWiFiSuccess == WIFI_Ping(lanAddress, count, interval_mS) );

	/* Ping WAN device to confirm Internet access
	 * Note: an invalid domain name takes ~10 seconds to timeout, therefore
	 * is being skipped for sake of efficiency */
	uint8_t wanAddress[4] = {};
	CHECK( eWiFiSuccess == WIFI_GetHostIP( (char *)server, wanAddress) );
	CHECK( eWiFiSuccess == WIFI_Ping(wanAddress, count, interval_mS) );

	/* Get the IP address assigned to this device */
	uint8_t deviceIpAddress[4] = {};
	CHECK( 0 == deviceIpAddress[0] );
	CHECK( 0 == deviceIpAddress[1] );
	CHECK( 0 == deviceIpAddress[2] );
	CHECK( 0 == deviceIpAddress[3] );
	CHECK( eWiFiSuccess == WIFI_GetIP(deviceIpAddress) );
	CHECK( 192 == deviceIpAddress[0] );
	CHECK( 168 == deviceIpAddress[1] );
	CHECK( 1   == deviceIpAddress[2] );
	CHECK( 1   <  deviceIpAddress[3] );

	/* Post test cleanup */
	CHECK( eWiFiSuccess == WIFI_Disconnect() );
	CHECK( eWiFiNotSupported == WIFI_Off() );

}


TEST(WiFiClient, readZero_Null)
{
	httpRequest();

	/* Various combinations that result in no data being read */
	uint8_t buf;
	CHECK( 0 == client.read( nullptr, 0 ) );
	CHECK( enl::Status::Error == client.status() );

	CHECK( 0 == client.read( nullptr, 100 ) );
	CHECK( enl::Status::Error == client.status() );

	CHECK( 0 == client.read( &buf, 0 ) );
	CHECK( enl::Status::Ready == client.status() );

	CHECK( 0 == client.read( NULL, 100 ) );
	CHECK( enl::Status::Error == client.status() );
}

TEST(WiFiClient, readNoDataAvailable)
{
	// No server request, so no data available

	uint8_t buf[10];
	CHECK( 0 == client.read( buf, 10 ) );
	CHECK( enl::Status::Ready == client.status() );

	CHECK( -1 == client.read() );
	CHECK( enl::Status::Ready == client.status() );
}


TEST(WiFiClient, readOneByte)
{
	httpRequest();

	/* Get the server's response, first 4-bytes, 1-byte at a time */
	CHECK( 'H' == client.read() );
	CHECK( enl::Status::Ready == client.status() );

	CHECK( 'T' == client.read() );
	CHECK( enl::Status::Ready == client.status() );

	CHECK( 'T' == client.read() );
	CHECK( enl::Status::Ready == client.status() );

	CHECK( 'P' == client.read() );
	CHECK( enl::Status::Ready == client.status() );
}

TEST(WiFiClient, readSome)
{
	httpRequest();

	uint8_t buf[10];
	CHECK( 10 == client.read( buf, 10 ) );
	CHECK( enl::Status::Ready == client.status() );
}


TEST(WiFiClient, readToLarge)
{
	httpRequest();

	/* AWS limited reads to 1200, with no internal packet handler */
	CHECK( 1200 == client.read( bigData, sizeof( bigData ) ) );
	CHECK( enl::Status::Ready == client.status() );
}


TEST(WiFiClient, writeZero_Null)
{
	/* Various combinations that result in no data being written */
	uint8_t buf;
	CHECK( 0 == client.write( (uint8_t *)nullptr, 0 ) );
	CHECK( enl::Status::Error == client.status() );

	CHECK( 0 == client.write( (uint8_t *)nullptr, 100 ) );
	CHECK( enl::Status::Error == client.status() );

	CHECK( 0 == client.write( &buf, 0 ) );
	CHECK( enl::Status::Ready == client.status() );

	CHECK( 0 == client.write( (uint8_t *)NULL, 100 ) );
	CHECK( enl::Status::Error == client.status() );
}

TEST(WiFiClient, writeOneByte)
{
	uint8_t buf[10] = "Hello";

	CHECK( 1 == client.write( &buf[0], 1 ) );
	CHECK( enl::Status::Ready == client.status() );

	CHECK( 1 == client.write( buf[1] ) );
	CHECK( enl::Status::Ready == client.status() );

	CHECK( 1 == client.write( buf[2] ) );
	CHECK( enl::Status::Ready == client.status() );

	CHECK( 1 == client.write( buf[3] ) );
	CHECK( enl::Status::Ready == client.status() );

	CHECK( 1 == client.write( buf[4] ) );
	CHECK( enl::Status::Ready == client.status() );

}

TEST(WiFiClient, writeSome)
{
	uint8_t buf[10] = "Hello";

	CHECK( 10 == client.write( buf, 10 ) );
	CHECK( enl::Status::Ready == client.status() );
}

TEST(WiFiClient, writeToLarge)
{
	/* AWS limited write to 1200, with no internal packet handler */
	CHECK( 1200 == client.write( bigData, sizeof( bigData ) ) );
	CHECK( enl::Status::Ready == client.status() );

}
