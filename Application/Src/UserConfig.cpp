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


/**
 * Overview:
 *  - Overall design supports multiple consumers and a single producer of configuration data
 *  - When setting new values, only the "stored" value is updated
 *  - When getting configuration data, a read-only copy is accessed. This copy is updated one-time
 *  during system initialization.
 *  - As designed, separate data spaces exist for consumers and producers. System is reentrant
 *  so long as the above limitations are observed.
 */

#include <cstdio>
#include <cstring>

#include "UserConfig.hpp"
#include "syscalls.h"


/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/

/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/



/**
 * @brief Reads one full object worth of configuration parameters from storage, if invalid,
 * initializes to known state.
 * @param dest Is the location to load configuration data into
 */
void UserConfig::GetConfig(Config_t *dest)
{
	FILE *handle = std::fopen(Device.storage, "rb");
	size_t status = std::fread(dest, sizeof(Config_t), 1, handle);

	/* Validate, when invalid initialize */
	if    ( (handle == nullptr)
		 || (std::ferror(handle) == 1 )
		 || (status == 0)
		 || (dest->tableSize != TableSize)
		 || (dest->tableVersion > TableVersion)
		 ) {
		std::memset(dest, 0, sizeof(Config_t));
		dest->tableVersion = TableVersion;
		dest->tableSize = TableSize;
	}

	std::fclose(handle);
}

/**
 * @brief Writes one full object worth of configuration parameters into storage.
 * @param  source Is the location to write configuration data from
 * @retval On success, true is returned. On error, false is returned.
 */
bool UserConfig::SetConfig(std::unique_ptr<Config_t> source)
{
	_ssize_t status = 1;

	FILE *handle = std::fopen(Device.storage, "wb");

 	if (handle != nullptr) {
		std::fwrite(source.get(), sizeof(Config_t), 1, handle);
		std::fflush(handle);
	 	status = std::ferror(handle);
	 	std::fclose(handle);
	}

	return (status == 1 ? false : true);
}



/**
 * @brief  Retrieves the current AWS settings
 * @param  dest Is the location to load the requested setting(s)
 */
const UserConfig::Aws_t & UserConfig::GetAwsConfig() const
{
	return config.aws;
}


/**
 * @brief  Retrieves the current WiFi settings
 * @param  dest Is the location to load the requested setting(s)
 */
const UserConfig::Wifi_t & UserConfig::GetWifiConfig() const
{
	return config.wifi;
}



/**
 * @brief  Stores a new AWS key
 * @param  newKey Is the value to be saved
 * @retval On success, true is returned. On error, false is returned.
 */
bool UserConfig::SetAwsKey(std::unique_ptr<Key_t> newKey)
{
	std::unique_ptr<Config_t> modify = std::make_unique<Config_t>();

	GetConfig( modify.get() );
	modify->aws.key.value = newKey->value;
	modify->aws.key.size = newKey->size;
	return SetConfig( std::move(modify) );
}


/**
 * @brief  Stores a new WiFi radio power-on state
 * @param  isWifiOn Is the value to be saved. True turns radio on, false turns radio off
 * @retval On success, true is returned. On error, false is returned.
 */
bool UserConfig::SetWifiOn(bool isWifiOn)
{
	std::unique_ptr<Config_t> modify = std::make_unique<Config_t>();

	GetConfig( modify.get() );
	modify->wifi.isWifiOn = isWifiOn;
	return SetConfig( std::move(modify) );
}


/**
 * @brief  Stores a new WiFi password
 * @param  password Is the value to be saved
 * @retval On success, true is returned. On error, false is returned.
 */
bool UserConfig::SetWifiPassword(const PasswordValue_t *password, size_t size)
{
	std::unique_ptr<Config_t> modify = std::make_unique<Config_t>();

	GetConfig( modify.get() );
	modify->wifi.password.value = *password;
	modify->wifi.password.size = size;
	return SetConfig( std::move(modify) );
}


/**
 * @brief  Stores a new WiFi SSID value
 * @param  ssid Is the value to be saved
 * @retval On success, true is returned. On error, false is returned.
 */
bool UserConfig::SetWifiSsid(const SsidValue_t *ssid, size_t size)
{
	std::unique_ptr<Config_t> modify = std::make_unique<Config_t>();

	GetConfig( modify.get() );
	modify->wifi.ssid.value = *ssid;
	modify->wifi.ssid.size = size;
	return SetConfig( std::move(modify) );
}

