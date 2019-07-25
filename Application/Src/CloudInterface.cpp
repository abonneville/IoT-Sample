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

#include <cstring>

#include "WiFiClient.hpp"
#include "ThreadConfig.hpp"
#include "UserConfig.hpp"
#include "CloudInterface.hpp"

#include "WiFiStation.hpp"

/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/
enl::WiFiStation WiFi;
enl::WiFiClient client(enl::Type::Udp);
//const char server[] = "www.google.com";
//unsigned int localPort = 80;

const char server[] = "0.us.pool.ntp.org";
unsigned int localPort = 123;      // local port to listen for UDP packets

bool notConnected = true;

static constexpr uint16_t TestBufferSize = 1000;
uint8_t buf[ TestBufferSize ];


const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
char packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets


/* Function prototypes -----------------------------------------------*/
void httpRequest();
void sendNTPpacket();
void parseNTPpacket();

/* TODO: delete after replacing demo */
extern "C" void vDevModeKeyProvisioning( void );
extern "C" void vStartMQTTEchoDemo( void );

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
	const UserConfig::Wifi_t &wifiConfig = userConfigHandle.GetWifiConfig();

	WiFi.begin(wifiConfig.ssid.value.data(),
			   wifiConfig.password.value.data(),
			   enl::WiFiSecurityType::WPA2);

    /* A simple example to demonstrate key and certificate provisioning in
     * microcontroller flash using PKCS#11 interface. This should be replaced
     * by production ready key provisioning mechanism. */
    //vDevModeKeyProvisioning();
	vStartMQTTEchoDemo();


	const char *version = WiFi.firmwareVersion();
	std::printf("\nFirmware: %s\n", version);

	enl::WiFiSecurityType type = WiFi.encryptionType();
	std::printf("Security: %d\n", (int)type);

//	uint8_t count = WiFi.scanNetworks();
//	std::printf("Scan count: %u\n", count);


	if (WiFi.status() == enl::WiFiStatus::WL_CONNECTED) {
		enl::IPAddress ip { 192, 168, 1, 1};
		enl::PingStatus status = WiFi.ping(ip);
		if (status == enl::PingStatus::WL_PING_SUCCESS) {
			std::printf("\nSuccess: IP ping\n");
			notConnected = true;
		}
		else
		{
			std::printf("\nFail: IP ping\n");
			while (1);
		}
	}

	size_t length = 0;
	while (true) {

		sendNTPpacket();

		Thread::Delay(1000);

		length = client.read(packetBuffer, NTP_PACKET_SIZE);
		if ( length > 0 )
		{
			parseNTPpacket();

		}
		else if ( length == 0 )
		{
			int32_t status = client.status();
			std::printf("\n **** Connection status: %ld ****\n", status );
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
	}
}

void sendNTPpacket()
{
	if ( notConnected )
	{
		notConnected = false;

		std::printf("Connecting to server...");
		if ( client.connect(server, localPort) )
		{
			enl::IPAddress ipAddress = client.remoteIP();
			std::printf("ok\n");
			std::printf("Server IP: ");
			std::printf("%u.", ipAddress[0] );
			std::printf("%u.", ipAddress[1] );
			std::printf("%u.", ipAddress[2] );
			std::printf("%u\n", ipAddress[3] );

			std::printf("Server port: %u\n", client.remotePort() );
		}
		else
		{
			std::printf("fail\n");
			while(1);
		}
	}


	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	std::memset(packetBuffer, 0, NTP_PACKET_SIZE);

	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12]  = 49;
	packetBuffer[13]  = 0x4E;
	packetBuffer[14]  = 49;
	packetBuffer[15]  = 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:

	client.write(packetBuffer, NTP_PACKET_SIZE);
}

void parseNTPpacket()
{
	//the timestamp starts at byte 40 of the received packet and is four bytes,
	// or two words, long. First, esxtract the two words:
	uint32_t secsSince1900 = 0;
	secsSince1900  = ( (uint32_t)packetBuffer[40] << 24);
	secsSince1900 |= ( (uint32_t)packetBuffer[41] << 16);
	secsSince1900 |= ( (uint32_t)packetBuffer[42] <<  8);
	secsSince1900 |= ( (uint32_t)packetBuffer[43] <<  0);

    // this is NTP time (seconds since Jan 1 1900):
//	std::printf("Seconds since Jan 1, 1900 = %lu\n", secsSince1900);

	// now convert NTP time into everyday time (since Jan 1, 1970):
	static constexpr uint32_t seventyYears = 2208988800UL;
	uint32_t epoch = secsSince1900 - seventyYears;
//	std::printf("Unix time = %lu\n", epoch);

	//Display hour, minute, second
	static constexpr uint32_t SecsPerDay = 86400;
	static constexpr uint32_t SecsPerHour = 3600;
	static constexpr uint32_t SecsPerMin = 60;

	std::printf("UTC time = ");
	std::printf("%lu:", (epoch % SecsPerDay ) / SecsPerHour );
	std::printf("%lu:", (epoch % SecsPerHour) / SecsPerMin );
	std::printf("%lu\n", (epoch % SecsPerMin ) );
}
