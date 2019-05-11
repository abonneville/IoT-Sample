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

#include "ticks.hpp"
#include "version.hpp"
#include "stm32l4xx_hal.h"

#include "UserConfig.hpp"
#include "ResponseInterface.hpp"

using namespace cpp_freertos;
using namespace std;

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

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
		case RESPONSE_MSG_AWS_STATUS:
			AwsStatusHandler();
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

		default:
			InvalidHandler();
			break;
		}

		PromptHandler();

	}

	std::fclose(stdout);
}



/**
 * @brief Reports the status for the AWS connection.
 */
void ResponseInterface::AwsStatusHandler(void)
{
	const UserConfig::Aws_t &aws = userConfigHandle.GetAwsConfig();

	std::printf("-- AWS Status --\n");
	std::printf("Key size: %u\n", aws.key.size);
	std::fflush(stdout);
}


/**
 * @brief Reports a list of available commands.
 */
static void HelpHandler(void)
{
	int width = 25;

	std::printf("All commands are case sensitive.\n");
	std::printf("%-*s %s\n", width, "aws key <field>","Sets the AWS key for connecting to the AWS cloud server.");
	std::printf("%-*s %s\n", width, "aws status","Reports status for the AWS connection.");
	std::printf("%-*s %s\n", width, "reset","Full processor reset; core and peripherals, as well as external modules.");
	std::printf("%-*s %s\n", width, "status","TBD");
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
 * @brief	TBD
 */
static void StatusHandler(void)
{
	std::printf("Status - not implemented.\n");
	std::fflush(stdout);
}


/**
 * @brief Reports the various software version numbers.
 */
static void VersionHandler(void)
{
#define APPLICATION_VERSION_STRING "1.0.x1"
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
	const UserConfig::Wifi_t &wifi = userConfigHandle.GetWifiConfig();

	std::printf("-- WiFi Status --\n");
	std::printf("SSID: %s\n", wifi.ssid.value.data() );
	std::printf("Password: %s\n", wifi.password.value.data() );
	std::printf("Radio is %s.\n", ( wifi.isWifiOn == true ? "ON" : "OFF") );
	std::fflush(stdout);
}


