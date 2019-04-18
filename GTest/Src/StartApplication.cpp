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

#include <cstring>
#include <cstdio>
#include <stdlib.h>

#include "gtest.h"
#include "thread.hpp"
#include "sysdbg.h"
#include "syscalls.h"

#include <StartApplication.hpp>
#include "CommandInterface.hpp"
#include "ResponseInterface.hpp"


//using namespace cpp_freertos;
using namespace std;


/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/
static char commandLineBuffer[128] = {};
//static ResponseInterface rspThread;
//static CommandInterface cmdThread = CommandInterface(rspThread);

class GTestThread : public cpp_freertos::Thread {

    public:

	GTestThread()
           : Thread("GTestThread", 500, 2)
        {
            Start();
        };

    protected:

        virtual void Run() {
        	errno = 0;
        	FILE *handleRx = std::freopen(Device.std_in, "r", stdin);
        	app_SetBuffer(handleRx);
        	FILE *handleTx = std::freopen(Device.std_out, "w", stdout);
        	app_SetBuffer(handleTx);

        	while (true) {
            	// Wait for USB link to be established
            	fgets(commandLineBuffer, sizeof(commandLineBuffer), handleRx);
            	printf("\n**** Starting test now! ****\n\n");
            	fflush(stdout);

            	printf("\n Results: %d \n", RUN_ALL_TESTS());
            	printf("**** Testing complete! ****\n");
            	fflush(stdout);
            }
        };

    private:
};


GTestThread thread1;

/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/




/**
  * @brief  The application entry point to C++
  * @note	This method is intended to be called after platform initialization is completed
  * 		in main()
  */
void StartApplication(void)
{
	/* Note: When the RTOS scheduler is started, the main stack pointer (MSP) is reset,
	 * effectively wiping out all stack/local main() variables and objects. Do not declare
	 * any C/C++ threads here. If objects need to be declared here, then change the code
	 * within prvPortStartFirstTask() to retain the MSP value.
	 */

	::testing::InitGoogleTest();

	cpp_freertos::Thread::StartScheduler();

	/* Infinite loop */
	while (1)
	{
	}
}

TEST(cleanLineBuffer, EmptyBuffer) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);
	testBuffer[0] = 0;
	EXPECT_EQ(&testBuffer[1], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
}

TEST(cleanLineBuffer, MissingNull) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);
	EXPECT_EQ(testBuffer.begin(), cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
}


TEST(cleanLineBuffer, LeadingWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	strncpy(testBuffer.data(), " Hello", testBuffer.size());
	EXPECT_EQ(&testBuffer[6], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello", testBuffer.data());

	strncpy(testBuffer.data(), "   Hello", testBuffer.size());
	EXPECT_EQ(&testBuffer[6], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello", testBuffer.data());
}


TEST(cleanLineBuffer, TrailingWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	strncpy(testBuffer.data(), "Hello ", testBuffer.size());
	EXPECT_EQ(&testBuffer[6], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello", testBuffer.data());

	strncpy(testBuffer.data(), "Hello   ", testBuffer.size());
	EXPECT_EQ(&testBuffer[6], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello", testBuffer.data());
}

TEST(cleanLineBuffer, MiddleWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	strncpy(testBuffer.data(), " Hello  World ", testBuffer.size());
	EXPECT_EQ(&testBuffer[12], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello World", testBuffer.data());

	strncpy(testBuffer.data(), "Hello   World", testBuffer.size());
	EXPECT_EQ(&testBuffer[12], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello World", testBuffer.data());
}


TEST(cleanLineBuffer, InvalidCharacters) {
	std::array<char, 256> testBuffer;
	testBuffer.fill(0x5A);

	/* Fill buffer with every possible (8-bit) combination */
	for (size_t i = 0; i < testBuffer.size(); i++) {
		testBuffer[i] = i + 1;
	}
	/* Set null terminator */
	testBuffer[testBuffer.size() - 1] = 0;

	EXPECT_EQ(&testBuffer[95], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", testBuffer.data());
}








TEST(validateBuffer, EmptyBuffer) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);
	testBuffer[0] = 0;
	EXPECT_EQ(&testBuffer[1], validateBuffer(testBuffer.begin(), testBuffer.end()));
}

TEST(validateBuffer, MissingNull) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);
	EXPECT_EQ(testBuffer.begin(), validateBuffer(testBuffer.begin(), testBuffer.end()));
}


TEST(validateBuffer, LeadingWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	strncpy(testBuffer.data(), " Hello\n", testBuffer.size());
	EXPECT_EQ(&testBuffer[7], validateBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello ", testBuffer.data());

	strncpy(testBuffer.data(), "   Hello\n", testBuffer.size());
	EXPECT_EQ(&testBuffer[7], validateBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello ", testBuffer.data());
}


TEST(validateBuffer, TrailingWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	strncpy(testBuffer.data(), "Hello \n", testBuffer.size());
	EXPECT_EQ(&testBuffer[7], validateBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello ", testBuffer.data());

	strncpy(testBuffer.data(), "Hello   \n", testBuffer.size());
	EXPECT_EQ(&testBuffer[7], validateBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello ", testBuffer.data());
}

TEST(validateBuffer, MiddleWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	strncpy(testBuffer.data(), " Hello  World \n", testBuffer.size());
	EXPECT_EQ(&testBuffer[13], validateBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello World ", testBuffer.data());

	strncpy(testBuffer.data(), "Hello   World\n", testBuffer.size());
	EXPECT_EQ(&testBuffer[13], validateBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello World ", testBuffer.data());
}















TEST(_SysMemCheckpoint, EvenHeap) {
	_SysMemState s1;
	_SysMemState s2;
	_SysMemState s3;
	_SysMemState s4;

	_SysMemCheckpoint(&s1);
	s2 = s1;
	EXPECT_EQ(0,_SysMemDifference(&s3, &s1, &s2));
	EXPECT_EQ(0,_SysMemDifference(&s4, &s1, &s2));
	EXPECT_EQ(0,_SysMemDifference(&s4, &s2, &s1));
	EXPECT_EQ(s3.freeHeapSizeNow, s4.freeHeapSizeNow);
	EXPECT_EQ(s3.freeHeapSizeMin, s4.freeHeapSizeMin);
}


TEST(_SysMemCheckpoint, OddHeap) {
	_SysMemState s1;
	_SysMemState s2;
	_SysMemState s3;
	_SysMemState s4;

	_SysMemCheckpoint(&s1);
	s2 = s1;
	s2.freeHeapSizeNow += 1;

	EXPECT_EQ(1,_SysMemDifference(&s3, &s1, &s2));
	EXPECT_EQ(1,_SysMemDifference(&s4, &s2, &s1));
	EXPECT_EQ(s3.freeHeapSizeNow, s4.freeHeapSizeNow);
	EXPECT_EQ(s3.freeHeapSizeMin, s4.freeHeapSizeMin);

	s2.freeHeapSizeNow -= 2;
	EXPECT_EQ(1,_SysMemDifference(&s3, &s1, &s2));
	EXPECT_EQ(1,_SysMemDifference(&s4, &s2, &s1));
	EXPECT_EQ(s3.freeHeapSizeNow, s4.freeHeapSizeNow);
	EXPECT_EQ(s3.freeHeapSizeMin, s4.freeHeapSizeMin);
}

TEST(_SysMemCheckpoint, FullHeap) {
	_SysMemState s1;
	_SysMemState s2;
	_SysMemState s3;

	s1.freeHeapSizeNow = 0;
	s1.freeHeapSizeMin = 0;
	s2 = s1;

	EXPECT_EQ(0,_SysMemDifference(&s3, &s1, &s2));
}



TEST(malloc, ZeroElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)malloc(0);
	EXPECT_EQ(ptr, nullptr);

	/* Zero size, no deallocation necessary */
	// free(ptr);
}

TEST(malloc, OneElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)malloc(1);
	EXPECT_NE(ptr, nullptr);

	free(ptr);
}

TEST(malloc, ManyElements){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)malloc(13);
	EXPECT_NE(ptr, nullptr);
	strcpy(ptr,"no leak now");

	free(ptr);
}


TEST(malloc, OutOfMemory){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)malloc(100000);
	EXPECT_EQ(ptr, nullptr);

	/* Insufficient memory, no deallocation necessary */
	// free(ptr);
}



TEST(realloc, ZeroElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)malloc(12);
	strcpy(ptr,"cool");
	char *nptr = (char *)realloc(ptr, 0);
	EXPECT_EQ(nptr, nullptr);

	free(ptr);
	/* Not required, zero size block */
	//free(nptr);
}


TEST(realloc, OneElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)malloc(12);
	strcpy(ptr,"cool");
	char *nptr = (char *)realloc(ptr, 1);
	EXPECT_NE(ptr, nptr);
	EXPECT_EQ(nptr[0], 'c');

	/* Not required, realloc already moved and freed old location */
	//free(ptr);
	free(nptr);
}


TEST(realloc, ManyElements){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)malloc(12);
	strcpy(ptr,"cool");
	char *nptr = (char *)realloc(ptr, 13);
	EXPECT_NE(ptr, nptr);
	ASSERT_STREQ("cool", nptr);

	/* Not required, realloc already moved and freed old location */
	//free(ptr);
	free(nptr);
}


TEST(realloc, NullPtr){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)realloc(NULL, 13);
	EXPECT_NE(ptr, nullptr);

	free(ptr);
}

TEST(realloc, NullPtr_ZeroElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)realloc(NULL, 0);
	EXPECT_EQ(ptr, nullptr);

	/* Not required, no memory allocated */
	//free(ptr);
	//free(nptr);
}


TEST(realloc, OutOfMemory){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)realloc(NULL, 100000);
	EXPECT_EQ(ptr, nullptr);

	/* Insufficient memory, no deallocation necessary */
	// free(ptr);
}



TEST(calloc, ZeroElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)calloc(0, 0);
	EXPECT_EQ(ptr, nullptr);

	ptr = (char *)calloc(0, 1);
	EXPECT_EQ(ptr, nullptr);

	ptr = (char *)calloc(1, 0);
	EXPECT_EQ(ptr, nullptr);

	/* Not required, no memory allocated */
	//free(ptr);
}


TEST(calloc, OneElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)calloc(1, 1);
	EXPECT_NE(ptr, nullptr);
	EXPECT_EQ(0, ptr[0]);

	free(ptr);
}


TEST(calloc, ManyElements){
	SysCheckMemory check = _SysCheckMemory();

	char val[20] = {};
	char *ptr = (char *)calloc(1,12);
	EXPECT_NE(ptr, nullptr);
	EXPECT_EQ(0, memcmp(val, ptr, 12));

	char *nptr = (char *)calloc(5,4);
	EXPECT_NE(nptr, nullptr);
	EXPECT_EQ(0, memcmp(val, nptr, 20));

	free(ptr);
	free(nptr);
}

TEST(calloc, OutOfMemory){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)calloc(1, 100000);
	EXPECT_EQ(ptr, nullptr);

	/* Insufficient memory, no deallocation necessary */
	// free(ptr);
}

