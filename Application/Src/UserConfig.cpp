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
 * @brief Reads configuration parameters from storage, if invalid, initializes to known
 * state.
 */
void UserConfig::GetConfig()
{
	FILE *handle = std::fopen(Device.storage, "rb");
	size_t status = std::fread(&config, sizeof(Config_t), 1, handle);

	/* Validate, when invalid initialize */
	if    ( (handle == nullptr)
		 || (std::ferror(handle) == 1 )
		 || (status == 0)
		 || (config.tableSize != TableSize)
		 || (config.tableVersion > TableVersion)
		 ) {
		std::memset(&config, 0, sizeof(Config_t));
		config.tableVersion = TableVersion;
		config.tableSize = TableSize;
	}

	std::fclose(handle);
}

/**
 * @brief Writes configuration parameters to storage.
 * @retval On success, true is returned. On error, false is returned.
 */
bool UserConfig::SetConfig()
{
	_ssize_t status = 1;

	FILE *handle = std::fopen(Device.storage, "wb");

 	if (handle != nullptr) {
		std::fwrite(&config, sizeof(Config_t), 1, handle);
		std::fflush(handle);
	 	status = std::ferror(handle);
	 	std::fclose(handle);
	}

	return (status == 1 ? false : true);
}


bool UserConfig::GetAwsConfig(const Aws_t &)
{
	cpp_freertos::LockGuard guard(AwsConfigGuard);

	return true;
}


void UserConfig::ReleaseAwsConfig()
{

}


bool UserConfig::GetWifiConfig(const Wifi_t &)
{
	cpp_freertos::LockGuard guard(WifiConfigGuardLock);

	return true;
}


void UserConfig::RelaseWifiConfig()
{

}



/**
 * @brief  Stores a new AWS key
 * @param  newKey Is the value to be saved
 * @retval On success, true is returned. On error, false is returned, and original value is retained.
 */
bool UserConfig::SetAwsKey(std::unique_ptr<Key_t> newKey, uint16_t size)
{
	std::unique_ptr<Key_t> backup = std::make_unique<Key_t>();

	/* When updating storage it can take 20+ mS to complete, so we do not want to block scheduling.
	 * Instead, utilize a semaphore to lock/unlock access when data is being adjusted.
	 */
	bool status = false;

	if (AwsConfigGuard.Lock(pdMS_TO_TICKS(100)) == true) {

		*backup = config.aws.key;
		config.aws.key = *newKey;

		status = SetConfig();
		if (status == false) {
			config.aws.key = *backup;
		}

		AwsConfigGuard.Unlock();
	}


	return status;
}


bool UserConfig::SetWifiOn(bool )
{
	cpp_freertos::LockGuard guard(WifiConfigGuardLock);

	return true;
}


bool UserConfig::SetWifiPassword(Password_t &password)
{
	cpp_freertos::LockGuard guard(WifiConfigGuardLock);

	return true;
}


bool UserConfig::SetWifiSsid(Ssid_t &ssid)
{
	cpp_freertos::LockGuard guard(WifiConfigGuardLock);

	return true;
}

