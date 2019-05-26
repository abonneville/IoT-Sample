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


#include <WiFiClient.hpp>
#include "ThreadConfig.hpp"
#include "UserConfig.hpp"
#include "CloudInterface.hpp"
#include "aws_wifi.h"



/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/
enl::WiFiClient client;
const char server[] = "www.google.com";
static constexpr uint16_t TestBufferSize = 1000;
uint8_t buf[ TestBufferSize ];
WIFINetworkParams_t networkParms = {};


/* Function prototypes -----------------------------------------------*/
void httpRequest();

/* External functions ------------------------------------------------*/


/**
 * @brief Creates the thread object, if the scheduler is already running, then then thread
 * will begin.
 */
CloudInterface::CloudInterface(UserConfig &uh)
	: Thread("CloudInterface", STACK_SIZE_CLOUD, THREAD_PRIORITY_NORMAL),
	  userConfigHandle(uh)
{
	Start();
}



/**
 * @brief Implements the persistent loop for thread execution.
 */
void CloudInterface::Run()
{

	/* Connect to the Internet via WiFi */
	WIFIReturnCode_t wifiRes;
	const UserConfig::Wifi_t &wifiConfig = userConfigHandle.GetWifiConfig();

	networkParms.pcPassword = wifiConfig.password.value.data();
	networkParms.ucPasswordLength = wifiConfig.password.size - 1;
	networkParms.pcSSID = wifiConfig.ssid.value.data();
	networkParms.ucSSIDLength = wifiConfig.ssid.size - 1;
	networkParms.xSecurity = eWiFiSecurityWPA2;

	wifiRes = WIFI_On();
	wifiRes = WIFI_ConnectAP(&networkParms);

	if (WIFI_IsConnected() == 1) {
		uint8_t ipAddress[4] = { 192, 168, 1, 1};
		wifiRes = WIFI_Ping(ipAddress, 3, 10);
		if (wifiRes == eWiFiSuccess) {
			std::printf("\nSuccess: IP ping\n");
		}
		else
		{
			std::printf("\nFail: IP ping\n");
		}
	}

	size_t length = 0;
	while (true) {

		/* Emulate a stalled server by alternating requests */
		httpRequest();

		Thread::Delay(1000);

		uint32_t count = 0;
		length = 1;
		while ( length > 0 )
		{
			length = client.read(buf, TestBufferSize - 1);
			buf[length] = '\0';         // End off string required by Serial.print
//				std::printf("%s", (char*)buf);
			count += length;
		}

		std::printf("Packet size: %lu\n", count);
		if ( count == 0 )
		{
			int32_t status = client.status();
			std::printf("Connection status: %ld\n", status );
		}

		Thread::Delay(5000);
	}
}



// this method makes a HTTP connection to the server:
void httpRequest() {
	char msg[100];
	int length = 0;
	// close any connection before send a new request.
	// This will free the socket on the WiFi module
	client.stop();

	// if there's a successful connection:
	if (client.connect(server, 80)) {
		std::printf("\n\nConnecting to server...\n");
		// Make a HTTP request:
		length = snprintf(msg, 100, "GET /search?q=arduino HTTP/1.1\r\n");
		client.write(msg, length);
		length = snprintf(msg, 100, "Host: www.google.com\r\n");
		client.write(msg, length);
		length = snprintf(msg, 100, "Connection: close\r\n");
		client.write(msg, length);
		length = snprintf(msg, 100, "\r\n");
		client.write(msg, length);

		// note the time that the connection was made:
		//    lastConnectionTime = millis();

	} else {
		// if you can't make a connection:
		std::printf("Connection failed");
		std::printf("...reconnecting now!\n");
		WIFI_ConnectAP(&networkParms);

	}
}

