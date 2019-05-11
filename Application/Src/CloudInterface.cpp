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


#include "ThreadConfig.hpp"
#include "UserConfig.hpp"
#include "CloudInterface.hpp"
#include "aws_wifi.h"


/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/

/* Function prototypes -----------------------------------------------*/

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

	WIFIReturnCode_t wifiRes;
	const UserConfig::Wifi_t &wifiConfig = userConfigHandle.GetWifiConfig();

	WIFINetworkParams_t networkParms = {};
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

	while (true) {
		Thread::Delay(1000);
	}
}
