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



#ifndef INCLUDE_WIFISTATION_HPP_
#define INCLUDE_WIFISTATION_HPP_

#include <stdint.h>
#include <memory>
#include "NetworkAddress.hpp"

namespace enl
{

enum class WiFiStatus
{
	WL_CONNECTED, /* assigned when connected to a WiFi network; */
	WL_NO_WIFI, /* assigned when no WiFi is present; */
	WL_IDLE_STATUS, /* it is a temporary status assigned when WiFi.begin() is called and
					 * remains active until the number of attempts expires
					 * (resulting in WL_CONNECT_FAILED) or a connection is established
					 * (resulting in WL_CONNECTED);
	 	 	 	 	 */
	WL_NO_SSID_AVAIL, /* assigned when no SSID are available; */
	WL_SCAN_COMPLETED, /* assigned when the scan networks is completed; */
	WL_CONNECT_FAILED, /* assigned when the connection fails for all the attempts; */
	WL_CONNECTION_LOST, /* assigned when the connection is lost; */
	WL_DISCONNECTED /* assigned when disconnected from a network; */
};

enum class PingStatus
{
	WL_PING_SUCCESS = 0, 			/* when the ping was successful */
	WL_PING_DEST_UNREACHABLE = -1, 	/* when the destination (IP or host is unreachable) */
	WL_PING_TIMEOUT = -2, 			/* when the ping times out */
	WL_PING_UNKNOWN_HOST = -3, 		/* when the host cannot be resolved via DNS */
	WL_PING_ERROR = -4				/* when an error occurs */
};

enum class WiFiSecurityType : int8_t
{
	Open  = 0, /* Open - No Security. */
	WEP   = 1, /* WEP Security. */
	WPA   = 2, /* WPA (TKIP) Security. */
	WPA2  = 3, /* WPA2(AES/CCMP) Security. */
	Auto  = 4, /* Uses WPA2, if unsuccessful, then WPA */

	/* Status only */
	Unknown = -1   /* Unknown type, not supported */
};

class WiFiStation
{
public:

	WiFiStation();
	WiFiStation(const WiFiStation &) = delete;
	WiFiStation & operator=(const WiFiStation &) = delete;
	~WiFiStation();

    WiFiStatus begin(const char * ssid);
    WiFiStatus begin(const char * ssid, const char * password, WiFiSecurityType type = WiFiSecurityType::Auto);
    void disconnect(void);
    WiFiStatus status();

    const char * firmwareVersion();
    IPAddress localIP();
    MACAddress & macAddress(MACAddress & mac);
    IPAddress subnetMask();
    IPAddress gatewayIP();

    const char * SSID();
    const char * SSID(uint8_t networkItem);
    MACAddress & BSSID(MACAddress & bssid);
    int32_t RSSI();
    int32_t RSSI(uint8_t networkItem);
    WiFiSecurityType encryptionType();
    WiFiSecurityType encryptionType(uint8_t networkItem);
    uint8_t scanNetworks();

    PingStatus ping(IPAddress ip, uint32_t timeout = 10);
    PingStatus ping(const char * host, uint32_t timeout = 10);
//    void setMac(uint8_t *macAddress);
//    static uint8_t getSocket();
//    static int hostByName(const char *aHostname, IPAddress aResult);

protected:

private:
    class impl;
    std::unique_ptr<impl> pimpl;

    void init();
};


} /* namespace enl */

#endif /* INCLUDE_WIFISTATION_HPP_ */
