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
#include "UserConfig.h"
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
 * @brief  Retrieves the current cloud settings
 */
const UserConfig::Cloud_t & UserConfig::GetCloudConfig() const
{
	return config.cloud;
}


/**
 * @brief  Retrieves the current WiFi settings
 */
const UserConfig::Wifi_t & UserConfig::GetWifiConfig() const
{
	return config.wifi;
}



/**
 * @brief  Stores a new cloud key
 * @param  newKey Is the value to be saved
 * @retval On success, true is returned. On error, false is returned.
 */
bool UserConfig::SetCloudKey(std::unique_ptr<Key_t> newKey)
{
	std::unique_ptr<Config_t> modify = std::make_unique<Config_t>();

	GetConfig( modify.get() );
	modify->cloud.key.value = newKey->value;
	modify->cloud.key.size = newKey->size;
	return SetConfig( std::move(modify) );
}


/**
 * @brief  Stores a new cloud certificate
 * @param  newCert Is the value to be saved
 * @retval On success, true is returned. On error, false is returned.
 */
bool UserConfig::SetCloudCert(std::unique_ptr<Cert_t> newCert)
{
	std::unique_ptr<Config_t> modify = std::make_unique<Config_t>();

	GetConfig( modify.get() );
	modify->cloud.cert.value = newCert->value;
	modify->cloud.cert.size = newCert->size;
	return SetConfig( std::move(modify) );
}


/**
 * @brief  Stores a new cloud endpoint URL to server/broker
 * @param  newEndpointUrl Is the value to be saved
 * @retval On success, true is returned. On error, false is returned.
 */
bool UserConfig::SetCloudEndpointUrl(const EndpointUrlValue_t * newEndpointUrl)
{
	std::unique_ptr<Config_t> modify = std::make_unique<Config_t>();

	GetConfig( modify.get() );
	modify->cloud.EndpointUrl.value = *newEndpointUrl;
	return SetConfig( std::move(modify) );
}


/**
 * @brief  Stores a new cloud thing name
 * @param  newThingName Is the value to be saved
 * @retval On success, true is returned. On error, false is returned.
 */
bool UserConfig::SetCloudThingName(const ThingNameValue_t * newThingName)
{
	std::unique_ptr<Config_t> modify = std::make_unique<Config_t>();

	GetConfig( modify.get() );
	modify->cloud.ThingName.value = *newThingName;
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
 * @param  size Is how many bytes long the password is
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



/**
 * @brief  Retrieves the current cloud key value
 * @param  handle is an object that contains the cloud settings
 * @param  key points to the cloud key value
 * @param  size is the key length in bytes
 */
extern "C" void GetCloudKey(UCHandle handle, const uint8_t ** key, const uint16_t ** size )
{
	const UserConfig::Cloud_t & cloud = handle->GetCloudConfig();
	*key = cloud.key.value.data();
	*size = &cloud.key.size;
}


/**
 * @brief  Retrieves the current cloud cert value
 * @param  handle is an object that contains the cloud settings
 * @param  cert points to the cloud cert value
 * @param  size is the cert length in bytes
 */
extern "C" void GetCloudCert(UCHandle handle, const uint8_t ** cert, const uint16_t ** size )
{
	const UserConfig::Cloud_t & cloud = handle->GetCloudConfig();
	*cert = cloud.cert.value.data();
	*size = &cloud.cert.size;
}


/**
 * @brief  Retrieves the current cloud endpoint URL
 * @param  handle is an object that contains the cloud settings
 * @param  url points to the cloud endpoint url value
 */
extern "C" void GetCloudEndpointUrl(UCHandle handle, const char ** url )
{
	const UserConfig::Cloud_t & cloud = handle->GetCloudConfig();
	*url = cloud.EndpointUrl.value.data();
}


/**
 * @brief  Retrieves the current cloud thing name
 * @param  handle is an object that contains the cloud settings
 * @param  name points to the cloud thing name value
 */
extern "C" void GetCloudThingName(UCHandle handle, const char ** name )
{
	const UserConfig::Cloud_t & cloud = handle->GetCloudConfig();
	*name = cloud.ThingName.value.data();
}

