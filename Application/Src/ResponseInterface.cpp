/*
 * ResponseInterface.cpp
 *
 *  Created on: Mar 27, 2019
 *      Author: Andrew
 */

#include <syscalls.h>
#include <ThreadConfig.hpp>
#include <string>
#include <cstdio>
#include <chrono>

#include "ticks.hpp"
#include "version.hpp"
#include "stm32l4xx_hal.h"

#include "UserConfig.hpp"
#include "ResponseInterface.hpp"
#include "WiFiStation.hpp"
#include "AppVersion.hpp"

using namespace cpp_freertos;
using namespace std;

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
extern enl::WiFiStation WiFi;

static void HelpHandler(void);
static void InvalidHandler(void);
static void PromptHandler(void);
static void StatusHandler(void);
static void VersionHandler(void);

/**
 * @brief Creates a thread and a message queue to handle requests to generate response
 * messages.
 */
ResponseInterface::ResponseInterface(cpp_freertos::Queue &mh, UserConfig &uh)
       : Thread("ResponseInterface", STACK_SIZE_RESPONSE, THREAD_PRIORITY_NORMAL),
		  msgHandle(mh),
		  userConfigHandle(uh)
{
	Start();
}

ResponseInterface::~ResponseInterface()
{

}


/**
 * @brief Implements the persistent loop for thread execution.
 */
void ResponseInterface::Run()
{
	errno = 0;
	FILE *handle = std::freopen(Device.std_out, "w", stdout);
	app_SetBuffer(handle);

	ResponseId_t msgId = RESPONSE_MSG_INVALID;

	while (true) {

		/* Wait until a request is made to generate a response message */
		msgHandle.Dequeue(&msgId);

		switch (msgId) {
		case RESPONSE_MSG_CLOUD_STATUS:
			CloudStatusHandler();
			break;

		case RESPONSE_MSG_HELP:
			HelpHandler();
			break;

		case RESPONSE_MSG_PROMPT:
			break;

		case RESPONSE_MSG_STATUS:
			StatusHandler();
			break;

		case RESPONSE_MSG_VERSION:
			VersionHandler();
			break;

		case RESPONSE_MSG_WIFI_STATUS:
			WifiStatusHandler();
			break;

		case RESPONSE_MSG_INVALID:
		default:
			InvalidHandler();
			break;
		}

		PromptHandler();

	}

	std::fclose(stdout);
}



/**
 * @brief Reports the status for the cloud connection.
 */
void ResponseInterface::CloudStatusHandler(void)
{
	const UserConfig::Cloud_t &cloud = userConfigHandle.GetCloudConfig();

	std::printf("-- Cloud Status --\n");
	std::printf("Key size: %u\n", cloud.key.size);
	std::fflush(stdout);
}


/**
 * @brief Reports a list of available commands.
 */
static void HelpHandler(void)
{
	int width = 25;

	std::printf("All commands are case sensitive.\n");
	std::printf("%-*s %s\n", width, "cloud cert <field>","Sets the device cert for connecting to a cloud server.");
	std::printf("%-*s %s\n", width, "cloud key <field>","Sets the private key for connecting to a cloud server.");
	std::printf("%-*s %s\n", width, "cloud name <field>","Sets the thing name for connecting to a cloud server.");
	std::printf("%-*s %s\n", width, "cloud url <field>","Sets the hostname/endpoint URL for connecting to a cloud server.");
	std::printf("%-*s %s\n", width, "cloud status","Reports status for the cloud connection.");
	std::printf("%-*s %s\n", width, "reset","Full processor reset; core and peripherals, as well as external modules.");
	std::printf("%-*s %s\n", width, "status","High level system information and status.");
	std::printf("%-*s %s\n", width, "version","Report application and library version numbers.");
	std::printf("%-*s %s\n", width, "wifi on/off","Immediately turns WiFi radio on or off.");
	std::printf("%-*s %s\n", width, "wifi password <field>","Set the password for connecting to a particular WiFi network.");
	std::printf("%-*s %s\n", width, "wifi ssid <field>","Set the SSID for connecting to a particular WiFi network.");
	std::printf("%-*s %s\n", width, "wifi status","Reports status for the WiFi connection.");
	std::fflush(stdout);
}


/**
 * @brief Reports a statement indicating invalid command.
 */
static void InvalidHandler(void)
{
	std::fputs("Invalid command -- try \"help\" for a list of commands.\n", stdout);
	std::fflush(stdout);
}



/**
 * @brief Displays message prompt for user input
 */
static void PromptHandler(void)
{
	std::fputs(">> ", stdout);
	std::fflush(stdout);
}


/**
 * @brief  Reports high level system status.
 */
static void StatusHandler(void)
{
	std::printf("-- System Status --\n");

	/* Report duration since system was first turned on */
	using seconds32 = std::chrono::duration<uint32_t, std::ratio<1> >;
	auto tp = std::chrono::time_point_cast<seconds32> ( std::chrono::steady_clock::now() );

	uint32_t epoch = tp.time_since_epoch().count();
	uint32_t hour = epoch/3600;
	uint32_t sec= epoch%3600;
	uint32_t min = sec/60;
	uint32_t sec1 = sec%60;

	std::printf("Uptime: ");
	std::printf("%.2lu:", hour );
	std::printf("%.2lu:", min );
	std::printf("%.2lu\n", sec1 );

	/* Report high level link status */
	if ( WiFi.RSSI() != 0 ) {
		std::printf("WiFi: %s, Connected, ", WiFi.SSID() );

		if ( enl::PingStatus::WL_PING_SUCCESS == WiFi.ping("www.google.com", 10) ) {
			std::printf("Internet Access\n");
		}
		else {
			std::printf("No Internet\n");
		}

	}
	else {
		std::printf("Not connected\n");

	}

	std::fflush(stdout);
}


/**
 * @brief Reports the various software version numbers.
 */
static void VersionHandler(void)
{
	uint32_t version[4] = {};
	std::printf("Device application, version: %s\n", APPLICATION_VERSION_STRING);
	std::printf("Device operating system, version:\n");
	std::printf("  - Kernel: %s\n", tskKERNEL_VERSION_NUMBER);
	std::printf("  - Wrapper: %s\n", CPP_WRAPPERS_VERSION_STRING);
	std::printf("  - C++ runtime: %lu\n", __cplusplus);
	version[0] = version[1] = version[2] = version[3] = HAL_GetHalVersion();
	version[0] = (version[0] >> 24U) & 0xFF;
	version[1] = (version[1] >> 16U) & 0xFF;
	version[2] = (version[2] >>  8U) & 0xFF;
	version[3] = (version[3] >>  0U) & 0xFF;
	std::printf("  - HAL: %lu.%lu.%lu.%lu\n", version[0], version[1], version[2], version[3]);
	std::fflush(stdout);
}


/**
 * @brief	Reports status of the WiFi connection.
 */
void ResponseInterface::WifiStatusHandler()
{
	int width = 25;

	std::printf("-- WiFi Status --\n");

	std::printf("%-*s %s\n", width, "SSID", WiFi.SSID() );
	std::printf("%-*s %li dB\n", width, "Signal strength", WiFi.RSSI() );

	std::printf("%-*s ", width, "Security type" );

	enl::WiFiSecurityType wifiSecurity = WiFi.encryptionType();

	switch (wifiSecurity)
	{
	case enl::WiFiSecurityType::Open:
		std::printf("Open - no security\n");
		break;

	case enl::WiFiSecurityType::WEP:
		std::printf("WEP Security\n");
		break;

	case enl::WiFiSecurityType::WPA:
		std::printf("WPA (TKIP) Security\n");
		break;

	case enl::WiFiSecurityType::WPA2:
		std::printf("WPA2 (AES/CCMP) Security\n");
		break;

	case enl::WiFiSecurityType::Auto:
		std::printf("Auto\n");
		break;

	case enl::WiFiSecurityType::Unknown:
	default:
		std::printf("Unknown\n");
		break;
	};

	enl::IPAddress ipAddress = WiFi.gatewayIP();
	std::printf("%-*s ", width, "Gateway IP" );
	std::printf("%u.", ipAddress[0] );
	std::printf("%u.", ipAddress[1] );
	std::printf("%u.", ipAddress[2] );
	std::printf("%u\n", ipAddress[3] );

	ipAddress = WiFi.subnetMask();
	std::printf("%-*s ", width, "Subnet mask" );
	std::printf("%u.", ipAddress[0] );
	std::printf("%u.", ipAddress[1] );
	std::printf("%u.", ipAddress[2] );
	std::printf("%u\n", ipAddress[3] );

	ipAddress = WiFi.localIP();
	std::printf("%-*s ", width, "Device IP" );
	std::printf("%u.", ipAddress[0] );
	std::printf("%u.", ipAddress[1] );
	std::printf("%u.", ipAddress[2] );
	std::printf("%u\n", ipAddress[3] );

	enl::MACAddress macAddress = WiFi.macAddress(macAddress);
	std::printf("%-*s ", width, "Device MAC" );
	std::printf("%.2X-", macAddress[0] );
	std::printf("%.2X-", macAddress[1] );
	std::printf("%.2X-", macAddress[2] );
	std::printf("%.2X-", macAddress[3] );
	std::printf("%.2X-", macAddress[4] );
	std::printf("%.2X\n", macAddress[5] );

	std::printf("%-*s %s\n", width, "Device firmware", WiFi.firmwareVersion() );

	std::fflush(stdout);
}


