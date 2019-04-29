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


#ifndef USERCONFIG_HPP_
#define USERCONFIG_HPP_

#include <memory>
#include "CommandInterface.hpp"
#include "thread.hpp"


class UserConfig
{
	friend CommandInterface;

	public:
		typedef std::array<uint8_t, 256> Key_t;
		typedef std::array<uint8_t, 32> Password_t;
		typedef std::array<uint8_t, 32> Ssid_t;

		typedef struct {
			Key_t key;
			uint16_t keySize;
		}Aws_t;

		typedef struct {
			bool isWifiOn;
			Password_t password;
			Ssid_t ssid;
		} Wifi_t;

		UserConfig() { GetConfig(); };

		bool GetAwsConfig(const Aws_t &);
		void ReleaseAwsConfig();

		bool GetWifiConfig(const Wifi_t &);
		void RelaseWifiConfig();

	protected:

	private:
		cpp_freertos::MutexStandard AwsConfigGuard;
		cpp_freertos::MutexStandard WifiConfigGuardLock;

		typedef struct {
			uint16_t tableSize;
			uint16_t tableVersion;
			Aws_t aws;
			Wifi_t wifi;
			uint32_t checksum;
		}Config_t;

		Config_t config;
		static constexpr uint16_t TableVersion = 1;
		static constexpr uint16_t TableSize  = sizeof(Config_t);

		void GetConfig();
		bool SetConfig();

		bool SetAwsKey(std::unique_ptr<Key_t> key, uint16_t size);

		bool SetWifiOn(bool );
		bool SetWifiPassword(Password_t &password);
		bool SetWifiSsid(Ssid_t &ssid);

};

#endif /* USERCONFIG_HPP_ */
