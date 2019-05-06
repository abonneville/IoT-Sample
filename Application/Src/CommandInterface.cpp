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

#include <syscalls.h>
#include <ThreadConfig.hpp>
#include <cstdio>
#include <algorithm>
#include <cstring>

#include "device.h"

#include "UserConfig.hpp"
#include "CommandInterface.hpp"



/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro ---------- ---------------------------------------------------*/
#define ParseCmdWord(strInter, cmd) 	std::equal(strInter, strInter + sizeof(cmd) - 1, cmd)
#define ParseCmdWordEnd(strInter, cmd) 	std::equal(strInter, strInter + sizeof(cmd)    , cmd)


/* Private variables ---------------------------------------------------------*/
extern class UserConfig userConfig;

static constexpr char cmdPrompt[] = "";
static constexpr char cmdAws[] = "aws ";
static constexpr char cmdHelp[] = "help";
static constexpr char cmdReset[] = "reset";
static constexpr char cmdStatus[] = "status";
static constexpr char cmdWifi[] = "wifi ";
static constexpr char cmdVersion[] = "version";

static constexpr char fieldKey[] = "key";
static constexpr char fieldOff[] = "off";
static constexpr char fieldOn[] = "on";
static constexpr char fieldPassword[] = "password ";
static constexpr char fieldSsid[] = "ssid ";
static constexpr char fieldStatus[] = "status";

/* Private function prototypes -----------------------------------------------*/


/**
 * @brief Binds the CommandInterface to the requested ResponseInterface, then creates
 * the thread. If the scheduler is already running, then then thread will begin
 * @param handle identifies which ResponseInterface will handle command response messaging
 */
CommandInterface::CommandInterface(cpp_freertos::Queue &mh, UserConfig &uh)
	: Thread("CommandInterface", STACK_SIZE_COMMANDS, THREAD_PRIORITY_ABOVE_NORMAL),
	  msgHandle(mh),
	  userConfigHandle(uh)
{
	Start();

}


/**
 * @brief Implements the persistent loop for thread execution.
 */
void CommandInterface::Run()
{
	errno = 0;
	FILE *handle = std::freopen(Device.std_in, "r", stdin);
	app_SetBuffer(handle);

	Buffer_t::const_iterator cmdBegin;
	Buffer_t::const_iterator lineEnd;
	ResponseId_t responseId;

    while (true) {

    	/* Wait for a user command */
    	std::fgets(commandLineBuffer.begin(), commandLineBuffer.size(), stdin);

    	/* Validate buffer formatting */
    	lineEnd = cleanLineBuffer(commandLineBuffer.begin(), commandLineBuffer.end());
    	if (lineEnd == commandLineBuffer.begin()) {
    		/* Discard, buffer contents were invalid */
    		continue;
    	}

    	responseId = RESPONSE_MSG_INVALID;

    	/* Parse and execute command handler */
    	switch (commandLineBuffer[0]) {

    	case 'a':
    		if (ParseCmdWord(commandLineBuffer.cbegin(), cmdAws)) {
    			responseId = AwsCmdHandler(commandLineBuffer.cbegin() + sizeof(cmdAws) - 1, lineEnd);
    		}
    		break;

    	case 'h':
    		if (ParseCmdWordEnd(commandLineBuffer.cbegin(), cmdHelp)) {
    			responseId = RESPONSE_MSG_HELP;
    		}
    		break;

    	case 'r':
    		if (ParseCmdWordEnd(commandLineBuffer.cbegin(), cmdReset)) {
    			responseId = ResetCmdHandler();
    		}
    		break;

    	case 's':
    		if (ParseCmdWordEnd(commandLineBuffer.cbegin(), cmdStatus)) {
    			responseId = RESPONSE_MSG_STATUS;
    		}
    		break;

    	case 'v':
    		if (ParseCmdWordEnd(commandLineBuffer.cbegin(), cmdVersion)) {
    			responseId = RESPONSE_MSG_VERSION;
    		}
    		break;

    	case 'w':
    		if (ParseCmdWord(commandLineBuffer.cbegin(), cmdWifi)) {
    			responseId = WifiCmdHandler(commandLineBuffer.cbegin() + sizeof(cmdWifi) - 1, lineEnd);
    		}
    		break;

    	case '\0':
    		if (ParseCmdWordEnd(commandLineBuffer.cbegin(), cmdPrompt)) {
    			responseId = RESPONSE_MSG_PROMPT;
    		}
    		break;


    	default:
    		break;
    	}


    	/* Request response message be sent to host */
		msgHandle.Enqueue(&responseId, (TickType_t)10 );


    	// Log any and all error states for future report back to host

/*

    	// USB enumeration between host and device can/will be delayed on startup, so cout buffer will need
    	// special handling to establish data stream
    	if (cout.fail()) {
    		cout.clear();
    	}
*/

    }

    std::fclose(stdin);
}


/**
 * @brief	Parse and execute the AWS commands.
 * @param first, last The range of elements to be evaluated for a valid command.
 * @retval  User response message to be sent.
 */
ResponseId_t CommandInterface::AwsCmdHandler(Buffer_t::const_iterator first, Buffer_t::const_iterator last)
{
	std::unique_ptr<UserConfig::Key_t> newKey = std::make_unique<UserConfig::Key_t>();
	Buffer_t::const_iterator lineEnd;
	size_t lineSize = 0;
	size_t keySize = 0;

	ResponseId_t responseId = RESPONSE_MSG_INVALID;

	switch (first[0]) {
	case 'k':
		if (ParseCmdWordEnd(first, fieldKey)) {
			/*
			 * At this point, we have validated the request to store a new key. Next we need to poll
			 * until we get the entire key. Key length can be 0 up to max length bytes.
			 */
			while (keySize < sizeof(UserConfig::Key_t) ){
				std::fgets(commandLineBuffer.begin(), commandLineBuffer.size(), stdin);

				lineEnd = std::find(commandLineBuffer.cbegin(), commandLineBuffer.cend(), '\n');
				if (lineEnd == commandLineBuffer.cend()) {
					/* invalid contents, missing terminator */
					keySize = 0;
					break;
				}

				lineSize = (lineEnd - commandLineBuffer.cbegin());
				if (lineSize == 0) {
					/* Transfer complete */
					break;
				}

				if ( (keySize + lineSize) <= sizeof(UserConfig::Key_t) ) {
					std::memcpy(newKey->value.data() + keySize, commandLineBuffer.cbegin(), lineSize);
					keySize += lineSize;
				}
				else {
					/* Key is to large, discard  */
					keySize = 0;
					break;
				}

			}

			if (keySize > 0) {
				newKey->size = keySize;
				if (userConfigHandle.SetAwsKey(std::move(newKey)) == true) {
					responseId = RESPONSE_MSG_PROMPT;
				}
			}
		}
		break;

	case 's':
		if (ParseCmdWordEnd(first, fieldStatus)) {
			responseId = RESPONSE_MSG_AWS_STATUS;
		}
		break;

	default:
		break;
	}

	return responseId;

#if 0
	volatile _ssize_t status;

	FILE *handle = std::fopen(Device.storage, "wb");

 	if (handle != nullptr) {
		Aws_t *old = new(Aws_t);
		std::memcpy(old->key, config.aws.key, sizeof(config.aws.key));
		std::memcpy(config.aws.key, "hello", sizeof("hello"));
		status = std::fwrite(&config, sizeof(Config_t), 1, handle);

		if ( (status != 1) || (std::ferror(handle) == 1) ) {
			std::memcpy(config.aws.key, old->key, sizeof(config.aws.key));
		}

		delete(old);
	}

	std::fclose(handle);

#endif


#if 0
	uint8_t data[] = {5, 4, 3, 2, 1};
	FILE *handle = std::fopen(Device.storage, "wb");
	app_SetBuffer(handle);


//	status = std::fwrite(data, sizeof(data[0]), 5, handle);
//	status = std::fwrite(data, sizeof(data[0]), 2049, handle); // 2049 on fwrite, -1 on fflush, __SERR set during fflush
//	status = std::fwrite(data,            2049,    1, handle); //    1 on fwrite, -1 on fflush, __SERR set during fflush
	status = std::fwrite(data, sizeof(data[0]), 9999, handle); // 3072 on fwrite,  0 on fflush, __SERR set during fwrite
	status = std::fflush(handle);
	status = std::ferror(handle);
//	std::clearerr(handle);
	status = std::fclose(handle);
	errno = errno;
#endif

#if 0
	uint8_t test[] = {5, 4, 3, 2, 1};
	uint8_t data[5] = {};
	FILE *handle = std::fopen(Device.storage, "wb");
//	status = std::setvbuf(handle, nullptr, _IONBF, 0);
//	app_SetBuffer(handle);
	status = std::fwrite(test, sizeof(test[0]), 1, handle);
	status = std::fwrite(&test[1], sizeof(test[0]), 4, handle);
	status = std::fclose(handle);

	handle = std::fopen(Device.storage, "r");

	status = std::fread(data, sizeof(data[0]), 2, handle);
	status = std::fread(&data[2], sizeof(data[0]), 3, handle);

	status = std::fclose(handle);

	status = std::memcmp(data, test, 5);
#endif

#if 0
	uint8_t test[13] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 0, 0, 0, 0};

	uint32_t result, check;
	result = crc_mpeg2(&test[0], &test[9]);

	check = result;
	test[12] = (check      ) & 0xFF;
	test[11] = (check >>  8) & 0xFF;
	test[10] = (check >> 16) & 0xFF;
	test[ 9] = (check >> 24) & 0xFF;

	result = crc_mpeg2(test, &test[13]);

	uint8_t test[17] = {0x00, 0x09, 0xFF , 0xFF, '1', '2', '3', '4', '5', '6', '7', '8', '9', 0 ,0, 0, 0};

	uint32_t result, check;
	result = crc_mpeg2(&test[0], &test[13]);

	check = result;
	test[16] = (check      ) & 0xFF;
	test[15] = (check >>  8) & 0xFF;
	test[14] = (check >> 16) & 0xFF;
	test[13] = (check >> 24) & 0xFF;

	result = crc_mpeg2(test, &test[17]);
#endif

}



/**
 * @brief	Parse and execute the reset command.
 * @param first, last The range of elements to be evaluated for a valid command.
 * @retval  User response message to be sent.
 */
ResponseId_t CommandInterface::ResetCmdHandler()
{
	return RESPONSE_MSG_INVALID;
}


/**
 * @brief	Parse and execute the Wifi commands.
 * @param first, last The range of elements to be evaluated for a valid command.
 * @retval  User response message to be sent.
 */
ResponseId_t CommandInterface::WifiCmdHandler(Buffer_t::const_iterator first, Buffer_t::const_iterator last)
{
	ResponseId_t responseId = RESPONSE_MSG_INVALID;

	switch (first[0]) {
	case 'o':
		if (ParseCmdWordEnd(first, fieldOff)) {
			if (userConfigHandle.SetWifiOn(false) == true) {
				responseId = RESPONSE_MSG_PROMPT;
			}
		}
		else if (ParseCmdWordEnd(first, fieldOn)) {
			if (userConfigHandle.SetWifiOn(true) == true) {
				responseId = RESPONSE_MSG_PROMPT;
			}
		}
		break;

	case 'p':
		if (ParseCmdWord(first, fieldPassword)) {
			/* Validate new password */
			UserConfig::Password_t *pwdBegin = (UserConfig::Password_t *)(first + sizeof(fieldPassword) - 1);
			UserConfig::Password_t *pwdEnd = (UserConfig::Password_t *)(last - 1);
			size_t pwdSize = (size_t)pwdEnd - (size_t)pwdBegin;

			if (pwdSize > sizeof(UserConfig::Password_t)) {
				/* Discard, password is to large */
				break;
			}

			/* Save new password */
			if (userConfigHandle.SetWifiPassword(pwdBegin) == true) {
				responseId = RESPONSE_MSG_PROMPT;
			}
		}
		break;

	case 's':
		if (ParseCmdWord(first, fieldSsid)) {
			/* Validate new ssid */
			UserConfig::Ssid_t *ssidBegin = (UserConfig::Ssid_t *)(first + sizeof(fieldSsid) - 1);
			UserConfig::Ssid_t *ssidEnd = (UserConfig::Ssid_t *)(last - 1);
			size_t size = (size_t)ssidEnd - (size_t)ssidBegin;

			if (size > sizeof(UserConfig::Ssid_t)) {
				/* Discard, SSID is to large */
				break;
			}

			/* Save new SSID */
			if (userConfigHandle.SetWifiSsid(ssidBegin) == true) {
				responseId = RESPONSE_MSG_PROMPT;
			}
		}
		else if (ParseCmdWordEnd(first, fieldStatus)) {
			responseId = RESPONSE_MSG_WIFI_STATUS;
		}
		break;

	default:
		break;
	}

	return responseId;
}


/**
  * @brief  Indicates if character is outside valid range of characters
  * @param  c character to be evaluated
  * @retval true when an invalid character is detected, false when character is within valid range
  */
bool invalidChar (char c)
{
	/*/ Valid range is all printable characters, or the null terminator */
	return !((c > 0x1F && c < 0x7F) || c == 0x00);
}


/**
 * @brief Remove all non-printable characters and duplicate white space.
 * @note  Requires buffer contents to be null-terminated.
 * @param first, last The range of elements to clean
 * @retval Output iterator to the element past the null element,
 * 			or "first" if missing null element
 */
template char *cleanLineBuffer<char *>(char *, char *);

template <typename bufIter>
bufIter cleanLineBuffer(bufIter first, bufIter last)
{
	bufIter lineEditEnd;

	/* Find null terminator, end of command string */
	lineEditEnd = std::find(first, last, 0);
	if (lineEditEnd == last) return first; /* invalid contents, missing null terminator */
	lineEditEnd++; /* next location after null, same as STL end() */

	/* Remove all non-printable character(s) */
	lineEditEnd = std::remove_if(first, lineEditEnd, invalidChar);

	/* Remove duplicate white space */
	lineEditEnd = std::unique(first, lineEditEnd,
			[](char c1, char c2){ return c1 == ' ' && c2 == ' '; });

	/* Remove leading white space */
	if (first[0] == ' ') {
    	lineEditEnd = std::move(first + 1, lineEditEnd, first);
	}

	/* Remove trailing white space */
	if ((lineEditEnd - 2) > first) {
		/* Not empty, check for trailing white space */
    	if (*(lineEditEnd - 2) == ' ') {
    		*(lineEditEnd - 2) = *(lineEditEnd - 1);
    		--lineEditEnd;
    	}
	}

	return lineEditEnd;
}

/**
 * @brief Verify buffer is null terminated, and each command word
 * 			is separated by at least one space character.
 * @param first, last The range of elements to clean
 * @retval Output iterator to the element past the null element,
 * 			or "first" if missing null element
 */
template char *validateBuffer<char *>(char *, char *);

template <typename bufIter>
bufIter validateBuffer(bufIter first, bufIter last)
{
	bufIter lineEditEnd;

	/* Find null terminator, end of command string */
	lineEditEnd = std::find(first, last, '\0');
	if (lineEditEnd == last) return first; /* invalid contents, missing terminator */
	lineEditEnd++; /* next location after terminator, same as STL end() */

	/* Remove duplicate white space(s) */
	lineEditEnd = std::unique(first, lineEditEnd,
			[](char c1, char c2){ return c1 == ' ' && c2 == ' '; });

	/* Remove leading white space */
	if (first[0] == ' ') {
    	lineEditEnd = std::move(first + 1, lineEditEnd, first);
	}

	/* Remove trailing white space */
	if ((lineEditEnd - 3) > first) {
		/* Not empty, check for trailing white space */
    	if (*(lineEditEnd - 3) == ' ') {
    		*(lineEditEnd - 3) = '\n';
    		*(lineEditEnd - 2) = '\0';
    		--lineEditEnd;
    	}
	}

	return lineEditEnd;
}


