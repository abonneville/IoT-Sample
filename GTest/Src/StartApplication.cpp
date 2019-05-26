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
#include <errno.h>

#include "gtest.h"
#include "thread.hpp"
#include "sysdbg.h"
#include "syscalls.h"
#include "device.h"

#include "StartApplication.hpp"
#include "UserConfig.hpp"
#include "CommandInterface.hpp"
#include "ResponseInterface.hpp"




/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/
static char commandLineBuffer[128] = {};

/* These buffers are for generating and capturing large data sets */
static uint8_t bigTest[BUFSIZ * 3];
static uint8_t bigData[BUFSIZ * 3];


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
            	std::fgets(commandLineBuffer, sizeof(commandLineBuffer), handleRx);
            	std::printf("\n**** Starting test now! ****\n\n");
            	std::fflush(stdout);

            	std::printf("\n Results: %d \n", RUN_ALL_TESTS());
            	std::printf("**** Testing complete! ****\n");
            	std::fflush(stdout);
            }
        };

    private:
};


GTestThread thread1;


class BaseTest : public ::testing::Test {
public:
	void SetUp()
	{
		errno = 0;
		SysCheckMemory check = _SysCheckMemory();
	}

	void TearDown()
	{
		EXPECT_EQ(errno, 0);
	}
protected:
private:
};

// TODO modify validateBuffer to use class BaseTest
class malloc : public BaseTest {};
class realloc : public BaseTest {};
class calloc : public BaseTest {};
class fopen : public BaseTest {};
class fwrite : public BaseTest {};
class fread : public BaseTest {};
class crc : public BaseTest {};
class uConfig : public BaseTest {};


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


#if 1
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

	std::strncpy(testBuffer.data(), " Hello", testBuffer.size());
	EXPECT_EQ(&testBuffer[6], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello", testBuffer.data());

	std::strncpy(testBuffer.data(), "   Hello", testBuffer.size());
	EXPECT_EQ(&testBuffer[6], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello", testBuffer.data());
}


TEST(cleanLineBuffer, TrailingWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	std::strncpy(testBuffer.data(), "Hello ", testBuffer.size());
	EXPECT_EQ(&testBuffer[6], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello", testBuffer.data());

	std::strncpy(testBuffer.data(), "Hello   ", testBuffer.size());
	EXPECT_EQ(&testBuffer[6], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello", testBuffer.data());
}

TEST(cleanLineBuffer, MiddleWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	std::strncpy(testBuffer.data(), " Hello  World ", testBuffer.size());
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

	std::strncpy(testBuffer.data(), " Hello\n", testBuffer.size());
	EXPECT_EQ(&testBuffer[7], validateBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello\n", testBuffer.data());

	std::strncpy(testBuffer.data(), "   Hello\n", testBuffer.size());
	EXPECT_EQ(&testBuffer[7], validateBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello\n", testBuffer.data());
}


TEST(validateBuffer, TrailingWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	std::strncpy(testBuffer.data(), "Hello \n", testBuffer.size());
	EXPECT_EQ(&testBuffer[7], validateBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello\n", testBuffer.data());

	std::strncpy(testBuffer.data(), "Hello   \n", testBuffer.size());
	EXPECT_EQ(&testBuffer[7], validateBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello\n", testBuffer.data());
}

TEST(validateBuffer, MiddleWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	std::strncpy(testBuffer.data(), " Hello  World \n", testBuffer.size());
	EXPECT_EQ(&testBuffer[13], validateBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello World\n", testBuffer.data());

	std::strncpy(testBuffer.data(), "Hello   World\n", testBuffer.size());
	EXPECT_EQ(&testBuffer[13], validateBuffer(testBuffer.begin(), testBuffer.end()));
	ASSERT_STREQ("Hello World\n", testBuffer.data());
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



TEST_F(malloc, ZeroElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(0);
	EXPECT_EQ(ptr, nullptr);

	/* Zero size, no deallocation necessary */
	// free(ptr);
}

TEST_F(malloc, OneElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(1);
	EXPECT_NE(ptr, nullptr);

	free(ptr);
}

TEST_F(malloc, ManyElements){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(13);
	EXPECT_NE(ptr, nullptr);
	std::strcpy(ptr,"no leak now");

	free(ptr);
}


TEST_F(malloc, OutOfMemory){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(100000);
	EXPECT_EQ(ptr, nullptr);

	/* Insufficient memory, no deallocation necessary */
	// free(ptr);
}



TEST_F(realloc, ZeroElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(12);
	std::strcpy(ptr,"cool");
	char *nptr = (char *)std::realloc(ptr, 0);
	EXPECT_EQ(nptr, nullptr);

	std::free(ptr);
	/* Not required, zero size block */
	//free(nptr);
}


TEST_F(realloc, OneElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(12);
	std::strcpy(ptr,"cool");
	char *nptr = (char *)std::realloc(ptr, 1);
	EXPECT_NE(ptr, nptr);
	EXPECT_EQ(nptr[0], 'c');

	/* Not required, realloc already moved and freed old location */
	//free(ptr);
	std::free(nptr);
}


TEST_F(realloc, ManyElements){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(12);
	strcpy(ptr,"cool");
	char *nptr = (char *)std::realloc(ptr, 13);
	EXPECT_NE(ptr, nptr);
	ASSERT_STREQ("cool", nptr);

	/* Not required, realloc already moved and freed old location */
	//free(ptr);
	std::free(nptr);
}


TEST_F(realloc, NullPtr){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::realloc(NULL, 13);
	EXPECT_NE(ptr, nullptr);

	std::free(ptr);
}

TEST_F(realloc, NullPtr_ZeroElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::realloc(NULL, 0);
	EXPECT_EQ(ptr, nullptr);

	/* Not required, no memory allocated */
	//std::free(ptr);
	//std::free(nptr);
}


TEST_F(realloc, OutOfMemory){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::realloc(NULL, 100000);
	EXPECT_EQ(ptr, nullptr);

	/* Insufficient memory, no deallocation necessary */
	// std::free(ptr);
}



TEST_F(calloc, ZeroElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::calloc(0, 0);
	EXPECT_EQ(ptr, nullptr);

	ptr = (char *)std::calloc(0, 1);
	EXPECT_EQ(ptr, nullptr);

	ptr = (char *)std::calloc(1, 0);
	EXPECT_EQ(ptr, nullptr);

	/* Not required, no memory allocated */
	//std::free(ptr);
}


TEST_F(calloc, OneElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::calloc(1, 1);
	EXPECT_NE(ptr, nullptr);
	EXPECT_EQ(0, ptr[0]);

	std::free(ptr);
}


TEST_F(calloc, ManyElements){
	SysCheckMemory check = _SysCheckMemory();

	char val[20] = {};
	char *ptr = (char *)std::calloc(1,12);
	EXPECT_NE(ptr, nullptr);
	EXPECT_EQ(0, memcmp(val, ptr, 12));

	char *nptr = (char *)std::calloc(5,4);
	EXPECT_NE(nptr, nullptr);
	EXPECT_EQ(0, memcmp(val, nptr, 20));

	std::free(ptr);
	std::free(nptr);
}

TEST_F(calloc, OutOfMemory){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::calloc(1, 100000);
	EXPECT_EQ(ptr, nullptr);

	/* Insufficient memory, no deallocation necessary */
	// std::free(ptr);
}



//#else

TEST_F(fopen, ReadFlags)
{
	errno = 0;
	FILE *handle = std::fopen(Device.storage, "r");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(errno, 0);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	handle = std::fopen(Device.storage, "r+");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(errno, 0);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	handle = std::fopen(Device.storage, "rb");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(errno, 0);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	handle = std::fopen(Device.storage, "rb+");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(errno, 0);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	handle = std::fopen(Device.storage, "r+b");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(errno, 0);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}


TEST_F(fopen, WriteFlags)
{
	errno = 0;
	FILE *handle = std::fopen(Device.storage, "w");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(errno, 0);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	handle = std::fopen(Device.storage, "w+");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(errno, 0);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	handle = std::fopen(Device.storage, "wb");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(errno, 0);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	handle = std::fopen(Device.storage, "wb+");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(errno, 0);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	handle = std::fopen(Device.storage, "w+b");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(errno, 0);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}


TEST_F(fopen, AppendFlags)
{
	/*
	 * lseek() syscall is not implemented at this time, therefore "append" functionality
	 * is not supported.
	 */
	errno = 0;
	FILE *handle = std::fopen(Device.storage, "a");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(errno, EINVAL);

	errno = 0;
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}


TEST_F(fopen, InvalidFlags)
{
	errno = 0;
	FILE *handle = std::fopen(Device.storage, "R");
	EXPECT_EQ(handle, nullptr);
	EXPECT_EQ(errno, EINVAL);

	errno = 0;
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}



TEST_F(fopen, AlreadyOpen)
{
	errno = 0;
	FILE *handle = std::fopen(Device.storage, "r");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(errno, 0);

	FILE *handle2 = std::fopen(Device.storage, "r");
	EXPECT_EQ(handle2, nullptr);
	EXPECT_EQ(errno, EACCES);

	errno = 0;
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}


TEST_F(fopen, AlreadyClosed)
{
	errno = 0;
	FILE *handle = std::fopen(Device.storage, "r");
	EXPECT_NE(handle, nullptr);

	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}


/***************************************************************************************/

TEST_F(fwrite, ZeroLength)
{
	uint8_t data[] = {1, 2, 3, 4, 5};
	FILE *handle = std::fopen(Device.storage, "w");
	EXPECT_NE(handle, nullptr);

	EXPECT_EQ(std::fwrite(data, sizeof(data[0]), 0, handle), 0u);

	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}

TEST_F(fwrite, OneByte)
{
	uint8_t data[] = {1, 2, 3, 4, 5};
	FILE *handle = std::fopen(Device.storage, "w");
	EXPECT_NE(handle, nullptr);

	EXPECT_EQ(std::fwrite(data, sizeof(data[0]), 1, handle), 1u);

	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}


TEST_F(fwrite, ManyBytes)
{
	uint8_t data[] = {1, 2, 3, 4, 5};
	FILE *handle = std::fopen(Device.storage, "w");
	EXPECT_NE(handle, nullptr);

	EXPECT_EQ(std::fwrite(data, sizeof(data[0]), 5, handle), 5u);

	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}


TEST_F(fwrite, ManyInts)
{
	uint32_t data[] = {1, 2, 3, 4, 5};
	FILE *handle = std::fopen(Device.storage, "w");
	EXPECT_NE(handle, nullptr);

	EXPECT_EQ(std::fwrite(data, sizeof(data[0]), 5, handle), 5u);

	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}

TEST_F(fwrite, ToLarge)
{
	uint8_t data[] = {1, 2, 3, 4, 5};
	FILE *handle = std::fopen(Device.storage, "w");
	EXPECT_NE(handle, nullptr);

	/* fwrite will fail, but the exact number returned is dependent on the size of the internal
	 * buffer. The value returned will likely be a multiple of BUFSIZE until storage was filled.
	 */
	EXPECT_GT(std::fwrite(data, sizeof(data[0]), 10000, handle), 5u);
	EXPECT_EQ(std::ferror(handle), 1);
	EXPECT_EQ(std::feof(handle), 0);
	EXPECT_EQ(errno, ENOSPC);

	errno = 0;
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(std::ferror(handle), 0);
	EXPECT_EQ(std::feof(handle), 0);
	EXPECT_EQ(errno, 0);
}


/***************************************************************************************/

TEST_F(fread, ZeroLength)
{
	uint8_t data[5] = {};
	FILE *handle = std::fopen(Device.storage, "r");
	EXPECT_NE(handle, nullptr);

	EXPECT_EQ(std::fread(data, sizeof(data[0]), 0, handle), 0u);
	EXPECT_EQ(std::ferror(handle), 0);
	EXPECT_EQ(errno, 0);

	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}


TEST_F(fread, OneByte)
{
	uint8_t test[] = {1, 2, 3, 4, 5};
	uint8_t data[5] = {};
	FILE *handle = std::fopen(Device.storage, "w");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(std::fwrite(test, sizeof(test[0]), 1, handle), 1u);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	handle = std::fopen(Device.storage, "r");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(std::fread(data, sizeof(data[0]), 1, handle), 1u);
	EXPECT_EQ(std::ferror(handle), 0);
	EXPECT_EQ(errno, 0);

	EXPECT_EQ(std::memcmp(test, data, 1), 0);

	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}


TEST_F(fread, ManyBytes)
{
	uint8_t test[] = {1, 2, 3, 4, 5};
	uint8_t data[5] = {};
	FILE *handle = std::fopen(Device.storage, "w");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(std::fwrite(test, sizeof(test[0]), 5, handle), 5u);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	handle = std::fopen(Device.storage, "r");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(std::fread(data, sizeof(data[0]), 5, handle), 5u);
	EXPECT_EQ(std::ferror(handle), 0);
	EXPECT_EQ(errno, 0);

	EXPECT_EQ(std::memcmp(test, data, 5), 0);

	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}

#if 0 // Not supported, with buffering off only 1-byte / 8-byte row of flash is available
TEST_F(fread, NoBuffering)
{
	uint8_t test[] = {1, 2, 3, 4, 5};
	uint8_t data[5] = {};
	FILE *handle = std::fopen(Device.storage, "w");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(std::setvbuf(handle, nullptr, _IONBF, 0), 0);
	EXPECT_EQ(std::fwrite(test, sizeof(test[0]), 5, handle), 5u);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	handle = std::fopen(Device.storage, "r");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(std::setvbuf(handle, nullptr, _IONBF, 0), 0);
	EXPECT_EQ(std::fread(data, sizeof(data[0]), 5, handle), 5u);
	EXPECT_EQ(std::ferror(handle), 0);
	EXPECT_EQ(errno, 0);

	EXPECT_EQ(std::memcmp(test, data, 5), 0);

	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}
#endif



TEST_F(fread, ManyInts)
{
	uint32_t test[] = {1, 2, 3, 4, 5};
	uint32_t data[5] = {};
	FILE *handle = std::fopen(Device.storage, "w");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(std::fwrite(test, sizeof(test[0]), 5, handle), 5u);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	handle = std::fopen(Device.storage, "r");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(std::fread(data, sizeof(data[0]), 5, handle), 5u);
	EXPECT_EQ(std::ferror(handle), 0);
	EXPECT_EQ(errno, 0);

	EXPECT_EQ(std::memcmp(test, data, 5), 0);

	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
}


TEST_F(fread, MultiBuffer)
{
	/*
	 * To verify sequential access is guaranteed across multiple transfers,
	 * this test forces multiple buffer transfers to access the memory.
	 */
	constexpr size_t TEST_SIZE = (BUFSIZ * 2);
	srand(11);
	for (uint32_t index = 0; index < TEST_SIZE; index++) {
		bigTest[index] = rand();
	}

	FILE *handle = std::fopen(Device.storage, "w");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(std::fwrite(bigTest, sizeof(bigTest[0]), TEST_SIZE, handle), TEST_SIZE);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	handle = std::fopen(Device.storage, "r");
	EXPECT_NE(handle, nullptr);
	EXPECT_EQ(std::fread(bigData, sizeof(bigData[0]), TEST_SIZE, handle), TEST_SIZE);
	EXPECT_EQ(std::ferror(handle), 0);
	EXPECT_EQ(std::feof(handle), 0);
	EXPECT_EQ(errno, 0);

	EXPECT_EQ(std::memcmp(bigTest, bigData, TEST_SIZE), 0);

	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(std::ferror(handle), 0);
	EXPECT_EQ(std::feof(handle), 0);
	EXPECT_EQ(errno, 0);
}


TEST_F(fread, ToLarge)
{
	FILE *handle = std::fopen(Device.storage, "r");
	EXPECT_NE(handle, nullptr);

	/* fread will fail, but the exact number returned is dependent on the size of the internal
	 * buffer. The value returned will likely be a multiple of BUFSIZ until storage was exhausted.
	 */
	EXPECT_GT(std::fread(bigData, sizeof(bigData[0]), BUFSIZ * 4, handle), 5u);
	EXPECT_EQ(std::ferror(handle), 0);
	EXPECT_EQ(std::feof(handle), 1);
	EXPECT_EQ(errno, 0);

	errno = 0;
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(std::ferror(handle), 0);
	EXPECT_EQ(std::feof(handle), 0);
	EXPECT_EQ(errno, 0);
}




/***************************************************************************************/
TEST_F(crc, Verification)
{
	uint8_t test[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

	EXPECT_EQ(crc_mpeg2(test, &test[9]), 0x0376E6E7u);
}



TEST_F(uConfig, Invalid)
{
	/* Initialize storage with invalid values */
	std::unique_ptr<UserConfig::Config_t> testValue = std::make_unique<UserConfig::Config_t>();
	std::memset(testValue.get(), 0x5A, UserConfig::TableSize);

	FILE *handle = std::fopen(Device.storage, "wb");
	EXPECT_EQ(std::fwrite(testValue.get(), UserConfig::TableSize, 1, handle), 1u);
	EXPECT_EQ(std::fflush(handle), 0);
	EXPECT_EQ(std::ferror(handle), 0);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);

	/* Validate interface initializes to known state */
	UserConfig testConfig; /* Object instantiation emulates a power-on event */
	const UserConfig::Aws_t &aws = testConfig.GetAwsConfig();
	const UserConfig::Wifi_t &wifi = testConfig.GetWifiConfig();

	std::memset(bigTest, 0x00, UserConfig::TableSize);
	EXPECT_EQ(std::memcmp(bigTest, &aws, sizeof(UserConfig::Aws_t)), 0);
	EXPECT_EQ(std::memcmp(bigTest, &wifi, sizeof(UserConfig::Wifi_t)), 0);
}


TEST_F(uConfig, SetValues)
{
	/**
	 * Normally the objects would be defined at compile time, but for test purposes runtime
	 * instantiation will be used. Runtime instantiation will emulate a power-on event.
	 */

	/* Initialize storage to a known state, invalid values. This forces the interface to set safe
	 * defaults that we will later change.
	 */
	std::unique_ptr<UserConfig::Config_t> testValue = std::make_unique<UserConfig::Config_t>();
	std::memset(testValue.get(), 0x5A, UserConfig::TableSize);

	FILE *handle = std::fopen(Device.storage, "wb");
	EXPECT_EQ(std::fwrite(testValue.get(), UserConfig::TableSize, 1, handle), 1u);
	EXPECT_EQ(std::fflush(handle), 0);
	EXPECT_EQ(std::ferror(handle), 0);
	EXPECT_EQ(std::fclose(handle), 0);
	EXPECT_EQ(errno, 0);
	testValue.release();

	/* Create test values */
	std::unique_ptr<UserConfig::Key_t> testKey = std::make_unique<UserConfig::Key_t>();
	constexpr UserConfig::KeyValue_t MyKey = {"MyKeyIsThis"};
	testKey->value = MyKey;
	testKey->size = sizeof(MyKey);
	constexpr UserConfig::PasswordValue_t testPassword = {"MyPassword123*"};
	constexpr UserConfig::SsidValue_t testSsid = {"CoolThingsSSID"};
	size_t pwdSize = std::strlen(testPassword.data());
	size_t ssidSize = std::strlen(testSsid.data());

	/* Set parameters to a new value */
	{
		std::unique_ptr<UserConfig> testConfig = std::make_unique<UserConfig>();
		EXPECT_EQ(testConfig->SetWifiOn(true), true);
		EXPECT_EQ(testConfig->SetWifiPassword( &testPassword, pwdSize ), true);
		EXPECT_EQ(testConfig->SetWifiSsid(  &testSsid, ssidSize ), true);
		EXPECT_EQ(testConfig->SetAwsKey( std::move(testKey) ), true);
		testConfig.release();
	}
	{
		std::unique_ptr<UserConfig> testConfig = std::make_unique<UserConfig>();
		const UserConfig::Wifi_t &wifi = testConfig->GetWifiConfig();
		EXPECT_EQ(wifi.isWifiOn, true);
		ASSERT_STREQ( wifi.password.value.data(), testPassword.data() );
		EXPECT_EQ( wifi.password.size, pwdSize );
		ASSERT_STREQ( wifi.ssid.value.data(), testSsid.data() );
		EXPECT_EQ( wifi.ssid.size, ssidSize );
		testConfig.release();
	}

}


#endif
