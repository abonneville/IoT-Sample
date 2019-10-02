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

#include "stm32l4xx_hal.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro ---------- ---------------------------------------------------*/
#define ParseCmdWord(strInter, cmd) 	std::equal(strInter, strInter + sizeof(cmd) - 1, cmd)
#define ParseCmdWordEnd(strInter, cmd) 	std::equal(strInter, strInter + sizeof(cmd)    , cmd)


/* Private variables ---------------------------------------------------------*/
extern class UserConfig userConfig;

static constexpr char cmdPrompt[] = "";
static constexpr char cmdCloud[] = "cloud ";
static constexpr char cmdHelp[] = "help";
static constexpr char cmdReset[] = "reset";
static constexpr char cmdStatus[] = "status";
static constexpr char cmdWifi[] = "wifi ";
static constexpr char cmdVersion[] = "version";

static constexpr char fieldCert[] = "cert";
static constexpr char fieldKey[] = "key";
static constexpr char fieldName[] = "name ";
static constexpr char fieldOff[] = "off";
static constexpr char fieldOn[] = "on";
static constexpr char fieldPassword[] = "password ";
static constexpr char fieldSsid[] = "ssid ";
static constexpr char fieldStatus[] = "status";
static constexpr char fieldUrl[] = "url ";

/* Private function prototypes -----------------------------------------------*/


/**
 * @brief Binds the CommandInterface to the helper objects, then creates
 * the thread. If the scheduler is already running, then then thread will begin
 * @param mh handle for passing response messages
 * @param uh handle for accessing configuration data
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

    	case 'c':
    		if (ParseCmdWord(commandLineBuffer.cbegin(), cmdCloud)) {
    			responseId = CloudCmdHandler(commandLineBuffer.cbegin() + sizeof(cmdCloud) - 1, lineEnd);
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


    	// TODO: Log any and all error states for future report back to host


    }

    std::fclose(stdin);
}


/**
 * @brief	Parse and execute cloud server configuration commands.
 * @param first, last The range of elements to be evaluated for a valid command.
 * @retval  User response message to be sent.
 */
ResponseId_t CommandInterface::CloudCmdHandler(Buffer_t::const_iterator first, Buffer_t::const_iterator last)
{
	ResponseId_t responseId = RESPONSE_MSG_INVALID;

	switch (first[0]) {
	case 'c':
		if (ParseCmdWordEnd(first, fieldCert)) {
			/*
			 * At this point, we have validated the request to store a new certificate. Next we need to poll
			 * until we get the entire certificate. Certificate length can be 0 up to max length bytes.
			 */
			std::unique_ptr<UserConfig::Cert_t> newCert = std::make_unique<UserConfig::Cert_t>();

			newCert->size = RxPEMObject(newCert->value.data(), (uint16_t)newCert->value.size() );

			if ( newCert->size > 0 ) {
				if (userConfigHandle.SetCloudCert(std::move(newCert)) == true) {
					responseId = RESPONSE_MSG_PROMPT;
				}
			}

			return responseId;
		}
		break;

	case 'k':
		if (ParseCmdWordEnd(first, fieldKey)) {
			/*
			 * At this point, we have validated the request to store a new key. Next we need to poll
			 * until we get the entire key. Key length can be 0 up to max length bytes.
			 */
			std::unique_ptr<UserConfig::Key_t> newKey = std::make_unique<UserConfig::Key_t>();

			newKey->size = RxPEMObject(newKey->value.data(), (uint16_t)newKey->value.size() );

			if ( newKey->size > 0 ) {
				if (userConfigHandle.SetCloudKey(std::move(newKey)) == true) {
					responseId = RESPONSE_MSG_PROMPT;
				}
			}

			return responseId;
		}
		break;

	case 'n':
		if (ParseCmdWord(first, fieldName)) {
			/* Validate new thing name */
			UserConfig::ThingNameValue_t *nameBegin = (UserConfig::ThingNameValue_t *)(first + sizeof(fieldName) - 1);
			UserConfig::ThingNameValue_t *nameEnd = (UserConfig::ThingNameValue_t *)(last - 1);
			size_t size = (size_t)nameEnd - (size_t)nameBegin;

			if (size > sizeof(UserConfig::ThingNameValue_t)) {
				/* Discard, thing name is to large */
				break;
			}

			/* Save new thing name */
			if (userConfigHandle.SetCloudThingName(nameBegin) == true) {
				responseId = RESPONSE_MSG_PROMPT;
			}
		}
		break;

	case 's':
		if (ParseCmdWordEnd(first, fieldStatus)) {
			responseId = RESPONSE_MSG_CLOUD_STATUS;
		}
		break;

	case 'u':
		if (ParseCmdWord(first, fieldUrl)) {
			/* Validate new hostname/endpoint url */
			UserConfig::EndpointUrlValue_t *urlBegin = (UserConfig::EndpointUrlValue_t *)(first + sizeof(fieldUrl) - 1);
			UserConfig::EndpointUrlValue_t *urlEnd = (UserConfig::EndpointUrlValue_t *)(last - 1);
			size_t size = (size_t)urlEnd - (size_t)urlBegin;

			if (size > sizeof(UserConfig::EndpointUrlValue_t)) {
				/* Discard, thing url is to large */
				break;
			}

			/* Save new endpoint url */
			if (userConfigHandle.SetCloudEndpointUrl(urlBegin) == true) {
				responseId = RESPONSE_MSG_PROMPT;
			}
		}
		break;

	default:
		break;
	}

	return responseId;
}



/**
 * @brief	Parse and execute the reset command.
 * @retval  User response message to be sent.
 */
ResponseId_t CommandInterface::ResetCmdHandler()
{
	HAL_NVIC_SystemReset();

	/* Should never get here */
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
/* TODO: under review, not yet implemented */
#if 0
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
#endif
		break;

	case 'p':
		if (ParseCmdWord(first, fieldPassword)) {
			/* Validate new password */
			UserConfig::PasswordValue_t *pwdBegin = (UserConfig::PasswordValue_t *)(first + sizeof(fieldPassword) - 1);
			UserConfig::PasswordValue_t *pwdEnd = (UserConfig::PasswordValue_t *)(last - 1);
			size_t size = (size_t)pwdEnd - (size_t)pwdBegin;

			if (size > sizeof(UserConfig::PasswordValue_t)) {
				/* Discard, password is to large */
				break;
			}

			/* Save new password */
			if (userConfigHandle.SetWifiPassword(pwdBegin, size) == true) {
				responseId = RESPONSE_MSG_PROMPT;
			}
		}
		break;

	case 's':
		if (ParseCmdWord(first, fieldSsid)) {
			/* Validate new ssid */
			UserConfig::SsidValue_t *ssidBegin = (UserConfig::SsidValue_t *)(first + sizeof(fieldSsid) - 1);
			UserConfig::SsidValue_t *ssidEnd = (UserConfig::SsidValue_t *)(last - 1);
			size_t size = (size_t)ssidEnd - (size_t)ssidBegin;

			if (size > sizeof(UserConfig::SsidValue_t)) {
				/* Discard, SSID is to large */
				break;
			}

			/* Save new SSID */
			if (userConfigHandle.SetWifiSsid(ssidBegin, size) == true) {
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
 * @brief	Buffers and handles receiving a PEM object
 * @param	buffer is the location to hold the PEM message as its received
 * @param   length is the maximum number of bytes that will fit in buffer
 * @retval	size of PEM object received, in bytes
 */
uint16_t CommandInterface::RxPEMObject(uint8_t *buffer, uint16_t length)
{
	Buffer_t::iterator lineEnd;
	int32_t lineSize = 0;
	int32_t pemSize = 0;

	while (pemSize < length ){
		std::fgets(commandLineBuffer.begin(), commandLineBuffer.size(), stdin);

		lineEnd = std::find(commandLineBuffer.begin(), commandLineBuffer.end(), '\n');
		if (lineEnd == commandLineBuffer.cend()) {
			/* invalid contents, missing terminator */
			pemSize = 0;
			break;
		}

		lineSize = (lineEnd - commandLineBuffer.cbegin());

		if (lineSize == 0) {
			/* Transfer complete */

			/* PEM format must be null terminated, insert null terminator */
			if ( (pemSize + 1) <= length ) {
				buffer[pemSize] = 0;
				pemSize++;
			}
			else {
				/* Message is to large, discard  */
				pemSize = 0;
			}

			break;
		}

		/* PEM format requires a '\n' at the end of each line, keep terminator.
		 */
		lineSize++;

		if ( (pemSize + lineSize) <= length ) {
			std::memcpy(buffer + pemSize, commandLineBuffer.cbegin(), lineSize);
			pemSize += lineSize;
		}
		else {
			/* Message is to large, discard  */
			pemSize = 0;
			break;
		}

	}

	return (uint16_t)pemSize;
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


