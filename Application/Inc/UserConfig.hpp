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
	public:
		typedef std::array<uint8_t, 256> KeyValue_t;
		typedef std::array<char, 32> PasswordValue_t;
		typedef std::array<char, 32> SsidValue_t;

		typedef struct {
			PasswordValue_t value;
			size_t size;
		}Password_t;

		typedef struct {
			SsidValue_t value;
			size_t size;
		} Ssid_t;

		typedef struct {
			KeyValue_t value;
			uint16_t size;
		}Key_t;

		typedef struct {
			Key_t key;
		}Aws_t;

		typedef struct {
			bool isWifiOn;
			Password_t password;
			Ssid_t ssid;
		} Wifi_t;

		typedef struct {
			uint16_t tableSize;
			uint16_t tableVersion;
			Aws_t aws;
			Wifi_t wifi;
			uint32_t checksum;
		}Config_t;

		static constexpr uint16_t TableVersion = 1;
		static constexpr uint16_t TableSize  = sizeof(Config_t);

		/* Note: this invokes file I/O from the "global" workspace before the kernel has been
		 * started. This means the CRT file I/O requests ~400+ bytes during library initialization.
		 * This heap memory remains allocated in the "global" workspace and is never used again. If
		 * memory constraints become critical, need to reassess initializing this object using
		 * an explicit call from a running thread.
		 */
		UserConfig() { GetConfig(&config); };

		const Aws_t & GetAwsConfig() const;
		const Wifi_t & GetWifiConfig() const;

		bool SetAwsKey(std::unique_ptr<Key_t> );
		bool SetWifiOn(bool );
		bool SetWifiPassword(const PasswordValue_t *, size_t );
		bool SetWifiSsid(const SsidValue_t *, size_t );

	protected:

	private:
		Config_t config;

		void GetConfig(Config_t *);
		bool SetConfig(std::unique_ptr<Config_t> );
};

#endif /* USERCONFIG_HPP_ */
