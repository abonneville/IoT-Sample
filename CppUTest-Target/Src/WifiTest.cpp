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

#include "CppUTest/TestHarness.h"
#include "FreeRTOS.h"

#include "TestConfig.hpp"
#include "thread.hpp"

#include "WiFiClient.hpp"
#include "WiFiStation.hpp"


/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/
extern uint8_t bigData[ BIG_DATA_SIZE ];
static enl::WiFiClient tcpClient;
static enl::WiFiClient udpClient(enl::Type::Udp);

/* WiFi */
static enl::WiFiStation WiFi;
static constexpr char password[] = "sunshine";
static constexpr char ssid[] = "Dev-2G";

/* TCP Client */
static constexpr char tcpServer[] = "www.google.com";
static constexpr unsigned int tcpPort = 80;

/* UDP Client */
static constexpr char udpServer[] = "pool.ntp.org";
//static constexpr char udpServer[] = "time.nist.gov";
static constexpr unsigned int udpPort = 123;
static constexpr int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

/* Function prototypes -----------------------------------------------*/
static void httpRequest();
static void sendNTPpacket();

/* External functions ------------------------------------------------*/

TEST_GROUP(wifi)
{
	void setup()
	{
		/* Each time a WiFi connection is established, it takes 4+ seconds to complete. To minimize impact
		 * on overall test time, a connection will be established here and not disconnected during test
		 * teardown(). WiFi disconnect will be evaluated under a separate test module.
		 */
		if ( enl::WiFiStatus::WL_DISCONNECTED == WiFi.status() )
		{
			/* Connect to the network via WiFi */
			CHECK( enl::WiFiStatus::WL_DISCONNECTED == WiFi.status() );
			CHECK( enl::WiFiStatus::WL_CONNECTED == WiFi.begin(ssid, password, enl::WiFiSecurityType::WPA2) );
			CHECK( enl::WiFiStatus::WL_CONNECTED == WiFi.status() );
			/* Just repeating to verify behavior */
			CHECK( enl::WiFiStatus::WL_CONNECTED == WiFi.begin(ssid, password, enl::WiFiSecurityType::WPA2) );
			CHECK( enl::WiFiStatus::WL_CONNECTED == WiFi.status() );
		}
	}

	void teardown()
	{
	}

};

TEST_GROUP(WiFiClient) {
	void setup()
	{
		/* Each time a WiFi connection is established, it takes 4+ seconds to complete. To minimize impact
		 * on overall test time, a connection will be established here and not disconnected during test
		 * teardown(). WiFi disconnect will be evaluated under a separate test module.
		 */
		if ( enl::WiFiStatus::WL_DISCONNECTED == WiFi.status() )
		{
			/* Connect to the network via WiFi */
			CHECK( enl::WiFiStatus::WL_DISCONNECTED == WiFi.status() );
			CHECK( enl::WiFiStatus::WL_CONNECTED == WiFi.begin(ssid, password, enl::WiFiSecurityType::WPA2) );
			CHECK( enl::WiFiStatus::WL_CONNECTED == WiFi.status() );
			/* Just repeating to verify behavior */
			CHECK( enl::WiFiStatus::WL_CONNECTED == WiFi.begin(ssid, password, enl::WiFiSecurityType::WPA2) );
			CHECK( enl::WiFiStatus::WL_CONNECTED == WiFi.status() );
		}


		/*
		 * TCP client
		 */
     	CHECK( enl::Status::Disconnected == tcpClient.status() );
		CHECK( false == tcpClient.connected() );
		CHECK( true == tcpClient.connect(tcpServer, tcpPort) );
		CHECK( true == tcpClient.connected() );
		CHECK( enl::Status::Ready == tcpClient.status() );
		CHECK( true == tcpClient );

		/*
		 * UDP Client
		 */
		CHECK( enl::Status::Disconnected == udpClient.status() );
		CHECK( false == udpClient.connected() );
		CHECK( true == udpClient.connect(udpServer, udpPort) );
		CHECK( true == udpClient.connected() );
		CHECK( enl::Status::Ready == udpClient.status() );
		CHECK( true == udpClient );
	}

	void teardown()
	{
		/* Note: if you add a "test" here and it fails, the tear down process is aborted
		 */
		tcpClient.stop();
		udpClient.stop();

		/* Teardown complete, verify client(s) is disconnected...
		 */
		CHECK( false == tcpClient.connected() );
		CHECK( enl::Status::Disconnected == tcpClient.status() );
		CHECK( false == tcpClient );

		CHECK( false == udpClient.connected() );
		CHECK( enl::Status::Disconnected == udpClient.status() );
		CHECK( false == udpClient );
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
	tcpClient.write(msg, length);
	length = snprintf(msg, 100, "Host: www.google.com\r\n");
	tcpClient.write(msg, length);
	length = snprintf(msg, 100, "Connection: close\r\n");
	tcpClient.write(msg, length);
	length = snprintf(msg, 100, "\r\n");
	tcpClient.write(msg, length);
}



static void sendNTPpacket()
{
	uint8_t packet[ NTP_PACKET_SIZE ];

	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	std::memset(packet, 0, NTP_PACKET_SIZE);

	packet[0] = 0b11100011;   // LI, Version, Mode
	packet[1] = 0;     // Stratum, or type of clock
	packet[2] = 6;     // Polling Interval
	packet[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packet[12]  = 49;
	packet[13]  = 0x4E;
	packet[14]  = 49;
	packet[15]  = 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:

	CHECK( NTP_PACKET_SIZE == udpClient.write(packet, NTP_PACKET_SIZE) );
}




TEST(wifi, parameters)
{
	CHECK( enl::WiFiSecurityType::WPA2 == WiFi.encryptionType() );

	/* Ping LAN device to confirm network access */
	enl::IPAddress routerIP {192, 168, 1, 1};
	CHECK( enl::PingStatus::WL_PING_SUCCESS == WiFi.ping(routerIP) );

	/* Ping WAN device to confirm Internet access
	 * Note: an invalid domain name takes ~10 seconds to timeout, therefore
	 * is being skipped for sake of efficiency */
	CHECK( enl::PingStatus::WL_PING_SUCCESS == WiFi.ping(tcpServer) );

	/* Get router IP */
	CHECK( routerIP == WiFi.gatewayIP() );

	/* Get local subnet mask */
	enl::IPAddress mask = WiFi.subnetMask();
	CHECK( mask[0] == 255 );
	CHECK( mask[1] == 255 );
	CHECK( mask[2] == 255 );
	CHECK( mask[3] == 0 );

	/* Access device MAC address.
	 * This is hardware specific, change in hardware will result in different MAC
	 */
	const uint8_t macCheck[] = {196, 127, 81, 6, 25, 208};
	enl::MACAddress mac;
	WiFi.macAddress(mac);
	CHECK ( mac == macCheck );

	/* Get device IP.
	 * LAN uses DHCP, so static address not assigned. Byte 3 will vary */
	enl::IPAddress ip = WiFi.localIP();
	CHECK( ip[0] == 192 );
	CHECK( ip[1] == 168 );
	CHECK( ip[2] == 1 );
	CHECK( ip[3] != 1 );

	/* Get Device Firmware */
	const char fwCheck[] { "C3.5.2.5.STM" };
	STRNCMP_EQUAL( fwCheck, WiFi.firmwareVersion(), sizeof(fwCheck) );
}


TEST(wifi, disconnect)
{
	/* Post test cleanup */
	CHECK( enl::WiFiStatus::WL_DISCONNECTED != WiFi.status() );
	WiFi.disconnect();
	CHECK( enl::WiFiStatus::WL_DISCONNECTED == WiFi.status() );
}


TEST(wifi, scanNetwork)
{
	/* Verify which network we are connected to */
	STRNCMP_EQUAL ( ssid, WiFi.SSID(), sizeof(ssid) );

	/* Verify access to router BSSID (MAC address) */
	const uint8_t bssidCheck[] = {230, 244, 198, 15, 115, 115};
	enl::MACAddress routerBSSID;
	WiFi.BSSID(routerBSSID);
	CHECK ( routerBSSID == bssidCheck );

	/* Verify the live RSSI is reportable for this device */
	int32_t rssi = WiFi.RSSI();
	CHECK( ((rssi < 0 ) && ( rssi > -100 )) );


	/* Generate local scan list.
	 * Locally, we have at least 2 active networks, plus neighboring networks.
	 * Results can vary from scan to scan. */
	uint8_t count = WiFi.scanNetworks();
	CHECK( count > 1 );

	/* Verify the SSID is reportable from scan list */
	const char * ssid = WiFi.SSID( 0u );
	CHECK( ssid[0] != 0 );


	/* Verify the logged RSSI is reportable from scan list */
	rssi = WiFi.RSSI( 0 );
	CHECK( ((rssi < 0 ) && ( rssi > -100 )) );

	/* Verify the security type is reportable from scan list */
	enl::WiFiSecurityType type = WiFi.encryptionType( 0 );
	CHECK( type != enl::WiFiSecurityType::Unknown );
}


TEST(WiFiClient, readZero_Null)
{
	httpRequest();

	/* Various combinations that result in no data being read */
	uint8_t buf;
	CHECK( 0 == tcpClient.read( (char *)nullptr, 0 ) );
	CHECK( enl::Status::Error == tcpClient.status() );

	CHECK( 0 == tcpClient.read( (char *)nullptr, 100 ) );
	CHECK( enl::Status::Error == tcpClient.status() );

	CHECK( 0 == tcpClient.read( &buf, 0 ) );
	CHECK( enl::Status::Ready == tcpClient.status() );

	CHECK( 0 == tcpClient.read( (char *)NULL, 100 ) );
	CHECK( enl::Status::Error == tcpClient.status() );
}

TEST(WiFiClient, readNoDataAvailable)
{
	// No server request, so no data available

	uint8_t buf[10];
	CHECK( 0 == tcpClient.read( buf, 10 ) );
	CHECK( enl::Status::Ready == tcpClient.status() );

	CHECK( -1 == tcpClient.read() );
	CHECK( enl::Status::Ready == tcpClient.status() );
}


TEST(WiFiClient, readOneByte)
{
	httpRequest();

	/* Get the server's response, first 4-bytes, 1-byte at a time */
	CHECK( 'H' == tcpClient.read() );
	CHECK( enl::Status::Ready == tcpClient.status() );

	CHECK( 'T' == tcpClient.read() );
	CHECK( enl::Status::Ready == tcpClient.status() );

	CHECK( 'T' == tcpClient.read() );
	CHECK( enl::Status::Ready == tcpClient.status() );

	CHECK( 'P' == tcpClient.read() );
	CHECK( enl::Status::Ready == tcpClient.status() );
}

TEST(WiFiClient, readSome)
{
	httpRequest();

	uint8_t buf[10];
	CHECK( 10 == tcpClient.read( buf, 10 ) );
	CHECK( enl::Status::Ready == tcpClient.status() );
}


TEST(WiFiClient, readToLarge)
{
	httpRequest();

	/* AWS limited reads to 1200, with no internal packet handler */
	CHECK( 1200 == tcpClient.read( bigData, sizeof( bigData ) ) );
	CHECK( enl::Status::Ready == tcpClient.status() );
}


TEST(WiFiClient, writeZero_Null)
{
	/* Various combinations that result in no data being written */
	uint8_t buf;
	CHECK( 0 == tcpClient.write( (uint8_t *)nullptr, 0 ) );
	CHECK( enl::Status::Error == tcpClient.status() );

	CHECK( 0 == tcpClient.write( (uint8_t *)nullptr, 100 ) );
	CHECK( enl::Status::Error == tcpClient.status() );

	CHECK( 0 == tcpClient.write( &buf, 0 ) );
	CHECK( enl::Status::Ready == tcpClient.status() );

	CHECK( 0 == tcpClient.write( (uint8_t *)NULL, 100 ) );
	CHECK( enl::Status::Error == tcpClient.status() );
}

TEST(WiFiClient, writeOneByte)
{
	uint8_t buf[10] = "Hello";

	CHECK( 1 == tcpClient.write( &buf[0], 1 ) );
	CHECK( enl::Status::Ready == tcpClient.status() );

	CHECK( 1 == tcpClient.write( buf[1] ) );
	CHECK( enl::Status::Ready == tcpClient.status() );

	CHECK( 1 == tcpClient.write( buf[2] ) );
	CHECK( enl::Status::Ready == tcpClient.status() );

	CHECK( 1 == tcpClient.write( buf[3] ) );
	CHECK( enl::Status::Ready == tcpClient.status() );

	CHECK( 1 == tcpClient.write( buf[4] ) );
	CHECK( enl::Status::Ready == tcpClient.status() );

}

TEST(WiFiClient, writeSome)
{
	uint8_t buf[10] = "Hello";

	CHECK( 10 == tcpClient.write( buf, 10 ) );
	CHECK( enl::Status::Ready == tcpClient.status() );
}

TEST(WiFiClient, writeToLarge)
{
	/* AWS limited write to 1200, with no internal packet handler */
	CHECK( 1200 == tcpClient.write( bigData, sizeof( bigData ) ) );
	CHECK( enl::Status::Ready == tcpClient.status() );

}


TEST(WiFiClient, tcp_remoteData)
{
	/* TCP server IP is obtained using a URL, the IP is different from time to time.
	 */
	CHECK( 80 == tcpClient.remotePort() );
	CHECK( enl::IP_Any != tcpClient.remoteIP() );
}


TEST(WiFiClient, emptyHostHame)
{
	/* AWS implementation causes a hard fault when the host name is an empty string.
	 * This test will verify fix in place to prevent system crash.
	 */
	enl::WiFiClient testClient(enl::Type::Tcp);
	CHECK( false == testClient.connect("", 80) );
}



/**
 * UDP Specific testing
 */

TEST(WiFiClient, udp_partialDatagram)
{

	sendNTPpacket();

	static constexpr int SegmentSize_1 = 10;
	static constexpr int SegmentSize_2 = NTP_PACKET_SIZE - SegmentSize_1;

	uint8_t buf[NTP_PACKET_SIZE];
	CHECK( SegmentSize_1 == udpClient.read( buf, SegmentSize_1 ) );
	CHECK( enl::Status::Ready == udpClient.status() );

	/* Partial read of a datagram (above read() ), results in loosing the remaining datagram.
	 * This read() is an attempt to get another datagram packet, which does not exist and
	 * times out. Note: AWS socket interface does not forward status correctly, due to how
	 * TLS was integrated.
	 */
	CHECK( 0 == udpClient.read( &buf[SegmentSize_1], SegmentSize_2 ) );
	CHECK( enl::Status::Ready == udpClient.status() );
}

TEST(WiFiClient, udp_fullDatagram)
{

	sendNTPpacket();

	CHECK( NTP_PACKET_SIZE == udpClient.read( bigData, NTP_PACKET_SIZE ) );
	CHECK( enl::Status::Ready == udpClient.status() );
}

TEST(WiFiClient, udp_remoteData)
{
	/* NTP server IP is obtained from a large pool, the IP is different every time the
	 * pool is accessed.
	 */
	CHECK( 123 == udpClient.remotePort() );
	CHECK( enl::IP_Any != udpClient.remoteIP() );
}
