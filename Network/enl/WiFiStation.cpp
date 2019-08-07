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

#include "WiFiStation.hpp"
#include <cstring>

#include "FreeRTOS.h"
#include "aws_wifi.h"
#include "es_wifi_conf.h"
#include "aws_secure_sockets.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "aws_system_init.h"

#ifdef __cplusplus
}
#endif /* extern "C" */

/* Typedef -----------------------------------------------------------*/


/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/

/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/

namespace enl
{


size_t safe_strlen(const char *str, size_t max_len);


/**
 * Pimpl idiom is being used to keep vendor/driver types out of the header, and avoid
 * header dependency.
 */
class WiFiStation::impl
{
public:
	impl() :
		scanBuffer{ {} },
		scanCount(0),
		wifiStatus(WiFiStatus::WL_NO_WIFI),
		networkSettings { {} }
	{};

	WIFIScanResult_t scanBuffer[ ES_WIFI_MAX_DETECTED_AP ];
	uint8_t scanCount;
    WiFiStatus wifiStatus;

    /* networkSettings is a cached copy, so only access values that are static between station
     * connect and station disconnect.
     */
	ES_WIFI_Network_t networkSettings;
};



/**
 * @brief  Initialize access to the WiFi module
 */
WiFiStation::WiFiStation()
	: pimpl{ std::make_unique<impl>() }
{
	/* Note: the init() method is not invoked here, because the WiFi driver requires that
	 * the RTOS be operational/scheduling. If its invoked here, and the object was not
	 * instantiated inside a thread the software would stall during constructor initialization.
	 * By keeping init() out of here, we can instantiate the object in/out of thread scope.
	 *
	 * Instantiating the object outside of thread scope makes unit testing easier.
	 */
}

/**
 * @brief  Close any open connections
 */
WiFiStation::~WiFiStation()
{
	WIFI_Disconnect();
}


/**
 * @brief Initialize WiFi module
 */
void WiFiStation::init()
{
	if (pimpl->wifiStatus == WiFiStatus::WL_NO_WIFI)
	{

		/**
		 * SYSTEM_Init() is used to initialize the AWS middleware; sockets, MQTT, etc...
		 */
		SYSTEM_Init();


		/* WIFI_On() requires the RTOS to be up and running, or it will stall trying to
		 * acquire resources. This call initializes the interface, but does not make a
		 * network connection.
		 */
		if ( eWiFiSuccess == WIFI_On() )
		{
			pimpl->wifiStatus = WiFiStatus::WL_DISCONNECTED;
		}
		else
		{
			pimpl->wifiStatus = WiFiStatus::WL_NO_WIFI;
		}
	}
}


/**
 * @brief  Initializes the WiFi library's network settings and provides the current status
 * @note   This method attempts to establish a connection with no security, open network
 * @param  ssid (Service Set Identifier) is the name of the WiFi network you want to connect to.
 * @retval Current state of connection
 */
WiFiStatus WiFiStation::begin(const char *ssid)
{
	return WiFiStation::begin(ssid, "", WiFiSecurityType::Open);
}


/**
 * @brief  Initializes the WiFi library's network settings and provides the current status
 * @param  ssid (Service Set Identifier) is the name of the WiFi network you want to connect to.
 * @param  passphrase Associated with ssid for secure login
 * @param  type Specifies the type of security to be used for connection.
 * @retval Current state of connection
 */
WiFiStatus WiFiStation::begin(const char *ssid, const char *passphrase, WiFiSecurityType type)
{

	WiFiStation::init();

	if ( ( ssid == nullptr )
	   ||( passphrase == nullptr ) )
	{
		return pimpl->wifiStatus;
	}

	size_t ssidLength = safe_strlen(ssid, wificonfigMAX_SSID_LEN);
	size_t passLength = safe_strlen(passphrase, wificonfigMAX_PASSPHRASE_LEN);

	if (  ( ssidLength == 0 )
	   || ( ssidLength == wificonfigMAX_SSID_LEN ) )
	{
		return pimpl->wifiStatus;
	}

	/* For open networks, no password is used. */
	if ( type == WiFiSecurityType::Open )
	{
		passLength = 0; /* force null at index 0 */
	}
	else
	{
		if (  ( passLength == 0 )
		   || ( passLength == wificonfigMAX_SSID_LEN ) )
		{
			return pimpl->wifiStatus;
		}
	}

	if ( pimpl->wifiStatus != WiFiStatus::WL_NO_WIFI )
	{
		WIFINetworkParams_t networkParms = {};
		networkParms.pcPassword = passphrase;
		networkParms.ucPasswordLength = passLength;
		networkParms.pcSSID = ssid;
		networkParms.ucSSIDLength = ssidLength;
		networkParms.xSecurity = (WIFISecurity_t)type;

		WIFIReturnCode_t wifiRes;
		wifiRes = WIFI_ConnectAP(&networkParms);

		if ( wifiRes == eWiFiSuccess )
		{
			WIFI_GetNetworkSettings( &pimpl->networkSettings );
			pimpl->wifiStatus = WiFiStatus::WL_CONNECTED;
		}
		else
		{
			std::memset( &pimpl->networkSettings, 0, sizeof( pimpl->networkSettings ) );
			pimpl->wifiStatus = WiFiStatus::WL_CONNECT_FAILED;
		}
	}

	return pimpl->wifiStatus;
}


/**
 * @brief Disconnects the WiFi from the current network.
 *
 */
void WiFiStation::disconnect()
{
	if (pimpl->wifiStatus != WiFiStatus::WL_NO_WIFI)
	{
		WIFI_Disconnect();
		pimpl->wifiStatus = WiFiStatus::WL_DISCONNECTED;

		/* Need to refresh network settings, because local IP, gateway, etc... are no longer
		 * valid
		 */
		WIFI_GetNetworkSettings( &pimpl->networkSettings );
	}
}


/**
 * @brief	Return the connection status
 * @retval  Current state of connection
 */
WiFiStatus WiFiStation::status()
{
	WiFiStation::init();
	return pimpl->wifiStatus;
}


/**
 * @brief  Report back the modules firmware version
 * @retval null-terminated char array (string)
 */
const char * WiFiStation::firmwareVersion()
{
	static uint8_t fwBuffer[ES_WIFI_FW_REV_SIZE] = {};

	WiFiStation::init();

	if ( pimpl->wifiStatus != WiFiStatus::WL_NO_WIFI )
	{
		WIFI_GetFirmwareVersion( fwBuffer );
	}

	return (char *)fwBuffer;
}


/**
 * @brief  Gets the station's local IP address, assigned by Access Point (AP)
 * @retval IP address for this station, 0 byte array if unavailable
 */
enl::IPAddress WiFiStation::localIP()
{
	IPAddress ip { pimpl->networkSettings.IP_Addr };
	return ip;
}


/**
 * @brief  Gets the stations subnet mask
 * @retval the subnet mask of the station, 0 byte array if unavailable
 */
enl::IPAddress WiFiStation::subnetMask()
{
	IPAddress mask { pimpl->networkSettings.IP_Mask };
	return mask;
}


/**
 * @brief  Gets the station's gateway/router IP address you are connected to
 * @retval IP address for gateway, 0 byte array if unavailable
 */
enl::IPAddress WiFiStation::gatewayIP()
{
	IPAddress ip { pimpl->networkSettings.Gateway_Addr };
	return ip;
}


/**
 * @brief  Gets the MAC Address for this station
 * @param  mac location to write MAC address for this station, 6 bytes long
 * @retval MAC address for this station, 0 byte array if unavailable
 */
enl::MACAddress & WiFiStation::macAddress(MACAddress & mac)
{
	std::memset(mac.data(), 0, mac.size());
	WIFI_GetMAC( mac.data() );
	return mac;
}


/**
 * @brief Gets the SSID of the current network
 * @retval A string containing the SSID the station is currently connected to.
 */
const char * WiFiStation::SSID()
{
	return (char *)pimpl->networkSettings.SSID;
}


/**
 * @brief  Report the SSID for a specific Access Point (AP)
 * @note   Use scanNetwork() to refresh list of available AP
 * @param  networkItem Specifies from which network to get the information
 * @retval A string containing the SSID, nullptr if not available.
 */
const char * WiFiStation::SSID(uint8_t networkItem)
{
	if ( networkItem < pimpl->scanCount )
	{
		return pimpl->scanBuffer[ networkItem ].cSSID;
	}

	return nullptr;
}


/**
 * @brief  Gets the MAC address of the gateway/router you are connected to
 * @param  bssid location to write MAC address for the gateway, 6 bytes long
 * @retval MAC address for gateway, 0 byte array if unavailable
 */
enl::MACAddress & WiFiStation::BSSID(MACAddress & bssid)
{
	if ( bssid != nullptr )
	{
		std::memset( bssid.data(), 0, 6);
		/* There is no direct way to get the gateway's MAC address, however, it is accessible
		 * from a network scan list. Since the gateway's location can change when another
		 * scan is performed, the MAC address will need to be copied into a destination.
		 */
		uint8_t count = scanNetworks();
		for (uint8_t index = 0; index < count; index++)
		{
			if ( std::strcmp(pimpl->scanBuffer[index].cSSID, (char *)pimpl->networkSettings.SSID) == 0 )
			{
				/* Match found, copy MAC address */
				for(uint8_t i = 0; i < 6; i++)
				{
					bssid[i] = pimpl->scanBuffer[index].ucBSSID[i];
				}
				break;
			}
		}
	}

	return bssid;
}


/**
 * @brief  Gets the signal strength for the connection between this station and the
 * 		   gateway/router
 * @retval The current Received Signal Strength in (RSSI) dBm, 0 if not available
 */
int32_t WiFiStation::RSSI()
{
	int32_t rssi;

	if ( eWiFiSuccess == WIFI_GetRSSI( &rssi ) )
	{
		return rssi;
	}

	return 0;
}

/**
 * @brief  Gets the signal strength for a specific Access Point (AP)
 * @note   Use scanNetwork() to refresh list of available AP and information
 * @param  networkItem Specifies from which network to get the information
 * @retval Last reported Received Signal Strength in (RSSI) dBm, 0 if not available
 */
int32_t WiFiStation::RSSI(uint8_t networkItem)
{
	if ( networkItem < pimpl->scanCount )
	{
		return pimpl->scanBuffer[ networkItem ].cRSSI;
	}

	return 0;
}


/**
 * @brief  Report the security type used by this station
 * @retval Security type currently in use
 */
WiFiSecurityType WiFiStation::encryptionType()
{
	return (WiFiSecurityType)pimpl->networkSettings.Security;
}



/**
 * @brief  Report the security type for a specific Access Point (AP)
 * @note   Use scanNetwork() to refresh list of available AP
 * @param  networkItem Specifies from which network to get the information
 * @retval Security type currently in use.
 */
WiFiSecurityType WiFiStation::encryptionType(uint8_t networkItem)
{

	if ( networkItem < pimpl->scanCount )
	{
		return (WiFiSecurityType)pimpl->scanBuffer[ networkItem ].xSecurity;
	}

	return WiFiSecurityType::Unknown;

}



/**
 * @brief  Scans for available WiFi networks and returns the discovered number.
 * @note   Updates the internal list of known Access Points, for latter retrieval of information.
 * @retval Number of discovered networks
 */
uint8_t WiFiStation::scanNetworks()
{
	uint8_t count = 0;

	WiFiStation::init();

	if ( pimpl->wifiStatus != WiFiStatus::WL_NO_WIFI )
	{
		std::memset( pimpl->scanBuffer, 0, sizeof(pimpl->scanBuffer) );
		WIFIReturnCode_t status = WIFI_Scan( pimpl->scanBuffer, ES_WIFI_MAX_DETECTED_AP );

		if ( status == eWiFiSuccess )
		{
			while( count < ES_WIFI_MAX_DETECTED_AP)
			{
				if ( pimpl->scanBuffer[count].cRSSI < 0)
				{
					++count;
				}
				else
				{
					break;
				}
			}
		}
	}

	if ( count == 0 )
	{
		pimpl->wifiStatus = WiFiStatus::WL_NO_SSID_AVAIL;
	}
	else
	{
		pimpl->wifiStatus = WiFiStatus::WL_SCAN_COMPLETED;
	}

	pimpl->scanCount = count;

	return count;
}


/**
 * @brief  	Ping the requested host.
 * @note	Troubleshooting utility to verify host is reachable
 * @param   ip IP address of remote host
 * @param	timeout In milliseconds to wait for each reply.
 * @retval	status code indicating ability to reach host
 */
PingStatus WiFiStation::ping(IPAddress ip, uint32_t timeout)
{
	WIFIReturnCode_t status = WIFI_Ping( (uint8_t *)ip.data(), 3, timeout);

	PingStatus retVal;

	switch ( status )
	{
	case eWiFiSuccess:
		retVal = PingStatus::WL_PING_SUCCESS;
		break;

	case eWiFiFailure:
		retVal = PingStatus::WL_PING_ERROR;
		break;

	case eWiFiTimeout:
		retVal = PingStatus::WL_PING_TIMEOUT;
		break;

	case eWiFiNotSupported:
		retVal = PingStatus::WL_PING_DEST_UNREACHABLE;
		break;

	default:
		retVal = PingStatus::WL_PING_ERROR;
		break;

	};

	return retVal;
}


/**
 * @brief  	Ping the requested host.
 * @note	Troubleshooting utility to verify host is reachable
 * @param   host URL
 * @param	timeout In milliseconds to wait for each reply.
 * @retval	status code indicating ability to reach host
 */
PingStatus WiFiStation::ping(const char * host, uint32_t timeout)
{
	IPAddress ip; // IP address of the host

	ip = SOCKETS_GetHostByName( host );

	if (ip == IP_None )
	{
		return PingStatus::WL_PING_DEST_UNREACHABLE;
	}

	return ping(ip, timeout);
}

/**
 * @brief   Current library does not contain strnlen, so using this workaround
 * @str     Character array to be assessed
 * @max_len max number of characters to examine
 * @retval  On success length of array, or max_len if not found
 */
size_t safe_strlen(const char *str, size_t max_len)
{
    const char * end = (const char *)std::memchr(str, '\0', max_len);
    if (end == NULL)
        return max_len;
    else
        return end - str;
}


} /* namespace enl */
