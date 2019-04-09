/*
 * CommandInterface.cpp
 *
 *  Created on: Mar 27, 2019
 *      Author: Andrew
 */
#include <ThreadConfig.hpp>
#include <cstdio>
#include <algorithm>
#include <cstring>

#include "CommandInterface.hpp"
#include "ResponseInterface.hpp"

//using namespace cpp_freertos;

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro ---------- ---------------------------------------------------*/
#define PARSE_ONE_WORD_CMD(buf, cmd) 	std::equal(buf.cbegin(), buf.cbegin() + sizeof(cmd), cmd)
#define PARSE_MULTI_WORD_CMD(buf, cmd) 	std::equal(buf.cbegin(), buf.cbegin() + sizeof(cmd) - 1, cmd)

/* Private variables ---------------------------------------------------------*/
static constexpr char cmdAws[] = "aws ";
static constexpr char cmdHelp[] = "help ";
static constexpr char cmdReset[] = "reset ";
static constexpr char cmdStatus[] = "status ";
static constexpr char cmdVersion[] = "version ";
static constexpr char cmdWifi[] = "wifi ";

static constexpr char fieldKey[] = "key ";
static constexpr char fieldOff[] = "off ";
static constexpr char fieldOn[] = "on ";
static constexpr char fieldPassword[] = "password ";
static constexpr char fieldSsid[] = "ssid ";
static constexpr char fieldStatus[] = "status ";

/* Private function prototypes -----------------------------------------------*/


/**
 * @brief Binds the CommandInterface to the requested ResponseInterface, then creates
 * the thread. If the scheduler is already running, then then thread will begin
 * @param handle identifies which ResponseInterface will handle command response messaging
 */
CommandInterface::CommandInterface(ResponseInterface &handle)
	: Thread("CommandInterface", STACK_SIZE_COMMANDS, THREAD_PRIORITY_ABOVE_NORMAL),
	  responseHandle(handle)
{
	Start();
}


/**
 * @brief Implements the persistent loop for thread execution.
 */
void CommandInterface::Run()
{
	Buffer_t::const_iterator cmdBegin;
//	buffer_t::const_iterator cmdEnd;
	Buffer_t::const_iterator lineEnd;

	ResponseId_t responseId;

    while (true) {

    	/* Wait for a user command */
    	std::fgets(commandLineBuffer.begin(), commandLineBuffer.size(), stdin);


    	/* Validate buffer formatting */
    	lineEnd = validateBuffer(commandLineBuffer.begin(), commandLineBuffer.end());
    	if (lineEnd == commandLineBuffer.begin()) {
    		/* Discard, buffer contents were invalid */
    		continue;
    	}

    	responseId = RESPONSE_MSG_INVALID;

    	/* Parse and execute command handler */
    	switch (commandLineBuffer[0]) {

    	case 'a':
    		if (PARSE_MULTI_WORD_CMD(commandLineBuffer, cmdAws)) {
    			responseId = AwsCmdHandler(commandLineBuffer.begin() + sizeof(cmdAws) - 1, lineEnd);
    		}
    		break;

    	case 'h':
    		if (PARSE_ONE_WORD_CMD(commandLineBuffer, cmdHelp)) {
    			responseId = RESPONSE_MSG_HELP;
    		}
    		break;

    	case 'r':
    		if (PARSE_ONE_WORD_CMD(commandLineBuffer, cmdReset)) {
    			responseId = ResetCmdHandler();
    		}
    		break;

    	case 's':
    		if (PARSE_ONE_WORD_CMD(commandLineBuffer, cmdStatus)) {
    			responseId = RESPONSE_MSG_STATUS;
    		}
    		break;

    	case 'v':
    		if (PARSE_ONE_WORD_CMD(commandLineBuffer, cmdVersion)) {
    			responseId = RESPONSE_MSG_VERSION;
    		}
    		break;

    	case 'w':
    		if (PARSE_MULTI_WORD_CMD(commandLineBuffer, cmdWifi)) {
    			responseId = WifiCmdHandler(commandLineBuffer.begin() + sizeof(cmdWifi) - 1, lineEnd);
    		}
    		break;

    	default:
    		break;
    	}


    	/* Request response message be sent to host */
		responseHandle.putResponse(responseId, (TickType_t)10);


    	// Log any and all error states for future report back to host

/*

    	// USB enumeration between host and device can/will be delayed on startup, so cout buffer will need
    	// special handling to establish data stream
    	if (cout.fail()) {
    		cout.clear();
    	}
*/

    }
}


/**
 * @brief	Parse and execute the AWS commands.
 * @param first, last The range of elements to be evaluated for a valid command.
 * @retval  User response message to be sent.
 */
ResponseId_t CommandInterface::AwsCmdHandler(Buffer_t::const_iterator first, Buffer_t::const_iterator last)
{
	return RESPONSE_MSG_AWS_STATUS;
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
	return RESPONSE_MSG_WIFI_STATUS;
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
	if (first[0] != 0) {
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
	lineEditEnd = std::find(first, last, 0);
	if (lineEditEnd == last) return first; /* invalid contents, missing null terminator */
	lineEditEnd++; /* next location after null, same as STL end() */

	/* All words need to have a trailing white space, including the last word.
	 * Swap the newline character with a space to guarantee a trailing white space exists.
	 *
	 * Bound the update within the range: first to last
	 */
	if ((lineEditEnd - 2) > first) {
		*(lineEditEnd - 2) = ' ';
	}

	/* Remove duplicate white space(s) */
	lineEditEnd = std::unique(first, lineEditEnd,
			[](char c1, char c2){ return c1 == ' ' && c2 == ' '; });

	/* Remove leading white space */
	if (first[0] == ' ') {
    	lineEditEnd = std::move(first + 1, lineEditEnd, first);
	}

	return lineEditEnd;
}


