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

#include "thread.hpp"
#include "sysdbg.h"
#include "syscalls.h"
#include "device.h"

#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/TestHarness.h"
#include "CppUTest/TestPlugin.h"
#include "CppUTest/TestRegistry.h"

#include "StartApplication.hpp"
#include "UserConfig.hpp"
#include "CommandInterface.hpp"
#include "ResponseInterface.hpp"

#include "TestConfig.hpp"

/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/


/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/

static char commandLineBuffer[128] = {};

/* These buffers are for generating and capturing large data sets */
uint8_t __attribute__((section(".bigData"))) bigTest[BIG_DATA_SIZE];
uint8_t __attribute__((section(".bigData"))) bigData[BIG_DATA_SIZE];


/* Function prototypes -----------------------------------------------*/
static void PrepNewlib( void );

/* External functions ------------------------------------------------*/


extern "C" {

UserConfig  userConfig;
}


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

	cpp_freertos::Thread::StartScheduler();

	/* Infinite loop */
	while (1)
	{
	}
}


/**
 * @brief Test/checks performed here run pre/post each and every test case
 */
class BaseTest : public TestPlugin {
public:
	BaseTest(const SimpleString& name)
	: TestPlugin(name)
	{
		/* Runs one-time before any testing is performed
		 */
	}

	~BaseTest()
	{
		/* Runs one-time after all testing has completed
		 */
	}

	virtual void preTestAction(UtestShell & /*test*/, TestResult & /*result*/)
	{
		/* Runs before each and every test case */

		/* Capture current heap allocation */
		_SysMemCheckpoint(&state1);

		errno = 0;
	}

	void postTestAction(UtestShell& test, TestResult& result)
	{
		/* Runs after each and every test case */

		/* Monitor for memory leaks
		 * Note: if the test case fails for another reason, a memory leak will be
		 * reported accidentally. Fix the previous failure before debugging memory leak.
		 */
		_SysMemCheckpoint(&state2);
		if( _SysMemDifference( &state3, &state1, &state2) ) {
			/* Memory leak detected, include in cpputest summary */
			TestFailure f(&test, "postTestAction");
			result.addFailure(f);

			std::printf("*** Error - Heap Memory Leak Detected ***\n");
			_SysMemDumpStatistics( &state3 );
		}

		CHECK_EQUAL(errno, 0);

	}
protected:
private:
	_SysMemState state1;
	_SysMemState state2;
	_SysMemState state3;
};


class CppUTestThread : public cpp_freertos::Thread {

    public:

	CppUTestThread()
           : Thread("CppUTestThread", (8192 / 4), 2)
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

        	/* Prepare optional library features */
        	PrepNewlib();

        	/* Register tests that run before and after every test case */
        	BaseTest bt("BaseTest");
        	TestRegistry::getCurrentRegistry()->installPlugin(&bt);

        	const char *argv[] = {"", "-v", NULL };
        	const int argc = sizeof( argv ) / sizeof( char* ) - 1;

        	while (true) {
            	// Wait for USB link to be established
            	std::fgets(commandLineBuffer, sizeof(commandLineBuffer), handleRx);
            	std::printf("\n**** Starting test now! ****\n\n");
            	std::fflush(stdout);

            	std::printf("\n Results: %d \n", RUN_ALL_TESTS(argc, argv));
            	std::printf("**** Testing complete! ****\n");
            	std::fflush(stdout);
            }
        };

    private:
};

static CppUTestThread thread1;



/**
 * @brief	By design, the newlib library reduces its default footprint size by not statically
 * allocating all its RAM requirements during compilation. If and only if an optional feature is
 * used, then the newlib library acquires the memory via the heap. This behavior impacts unit
 * testing, because when the memory is acquired for the first time, it is never released back
 * to the memory pool and results in a "false" memory leak being detected.
 *
 * This method is used to stimulate optional features prior to testing, so that the
 * heap memory is acquired prior to testing.
 *
 * @note	This has to be repeated for each test thread, since each thread maintains its own
 * control structures
 */
static void PrepNewlib( void )
{
	/* stdio is already addressed when the terminal interface is setup and used for reporting */

	/* strtok() */
	char str[] = "Brown foxes are cool";
	char *token = std::strtok( str, " " );
	volatile int count = 0;
	while ( token != nullptr )
	{
		++count;
		token = std::strtok( nullptr, " " );
	}

	/* rand() */
	std::srand ( count );
	volatile int randomValue = std::rand();
	++randomValue;

}

/* Every test group can have a setup and a teardown method. The setup method is called before
 * each test and the teardown method is called after each test.
 */
TEST_GROUP(cleanLineBuffer) {};
TEST_GROUP(validateBuffer) {};

TEST_GROUP(_SysMemCheckpoint) {};

TEST_GROUP(malloc) {};
TEST_GROUP(realloc) {};
TEST_GROUP(calloc) {};
TEST_GROUP(fopen) {};
TEST_GROUP(fwrite) {};
TEST_GROUP(fread) {
	void teardown()
	{
		/* Note: if you add a "test" here and it fails, the tear down process is aborted
		 */


		/* Teardown complete, go ahead and "test" as required
		 */
	}

};
TEST_GROUP(crc) {};
TEST_GROUP(uConfig) {};



TEST(cleanLineBuffer, EmptyBuffer) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);
	testBuffer[0] = 0;
	CHECK_EQUAL(&testBuffer[1], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
}

TEST(cleanLineBuffer, MissingNull) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);
	CHECK_EQUAL(testBuffer.begin(), cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
}


TEST(cleanLineBuffer, LeadingWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	std::strncpy(testBuffer.data(), " Hello", testBuffer.size());
	CHECK_EQUAL(&testBuffer[6], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	STRCMP_EQUAL("Hello", testBuffer.data());

	std::strncpy(testBuffer.data(), "   Hello", testBuffer.size());
	CHECK_EQUAL(&testBuffer[6], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	STRCMP_EQUAL("Hello", testBuffer.data());
}


TEST(cleanLineBuffer, TrailingWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	std::strncpy(testBuffer.data(), "Hello ", testBuffer.size());
	CHECK_EQUAL(&testBuffer[6], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	STRCMP_EQUAL("Hello", testBuffer.data());

	std::strncpy(testBuffer.data(), "Hello   ", testBuffer.size());
	CHECK_EQUAL(&testBuffer[6], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	STRCMP_EQUAL("Hello", testBuffer.data());
}

TEST(cleanLineBuffer, MiddleWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	std::strncpy(testBuffer.data(), " Hello  World ", testBuffer.size());
	CHECK_EQUAL(&testBuffer[12], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	STRCMP_EQUAL("Hello World", testBuffer.data());

	strncpy(testBuffer.data(), "Hello   World", testBuffer.size());
	CHECK_EQUAL(&testBuffer[12], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	STRCMP_EQUAL("Hello World", testBuffer.data());
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

	CHECK_EQUAL(&testBuffer[95], cleanLineBuffer(testBuffer.begin(), testBuffer.end()));
	STRCMP_EQUAL("!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", testBuffer.data());
}








TEST(validateBuffer, EmptyBuffer) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);
	testBuffer[0] = 0;
	CHECK_EQUAL(&testBuffer[1], validateBuffer(testBuffer.begin(), testBuffer.end()));
}

TEST(validateBuffer, MissingNull) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);
	CHECK_EQUAL(testBuffer.begin(), validateBuffer(testBuffer.begin(), testBuffer.end()));
}


TEST(validateBuffer, LeadingWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	std::strncpy(testBuffer.data(), " Hello\n", testBuffer.size());
	CHECK_EQUAL(&testBuffer[7], validateBuffer(testBuffer.begin(), testBuffer.end()));
	STRCMP_EQUAL("Hello\n", testBuffer.data());

	std::strncpy(testBuffer.data(), "   Hello\n", testBuffer.size());
	CHECK_EQUAL(&testBuffer[7], validateBuffer(testBuffer.begin(), testBuffer.end()));
	STRCMP_EQUAL("Hello\n", testBuffer.data());
}


TEST(validateBuffer, TrailingWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	std::strncpy(testBuffer.data(), "Hello \n", testBuffer.size());
	CHECK_EQUAL(&testBuffer[7], validateBuffer(testBuffer.begin(), testBuffer.end()));
	STRCMP_EQUAL("Hello\n", testBuffer.data());

	std::strncpy(testBuffer.data(), "Hello   \n", testBuffer.size());
	CHECK_EQUAL(&testBuffer[7], validateBuffer(testBuffer.begin(), testBuffer.end()));
	STRCMP_EQUAL("Hello\n", testBuffer.data());
}

TEST(validateBuffer, MiddleWhiteSpace) {
	std::array<char, 16> testBuffer;
	testBuffer.fill(0x5A);

	std::strncpy(testBuffer.data(), " Hello  World \n", testBuffer.size());
	CHECK_EQUAL(&testBuffer[13], validateBuffer(testBuffer.begin(), testBuffer.end()));
	STRCMP_EQUAL("Hello World\n", testBuffer.data());

	std::strncpy(testBuffer.data(), "Hello   World\n", testBuffer.size());
	CHECK_EQUAL(&testBuffer[13], validateBuffer(testBuffer.begin(), testBuffer.end()));
	STRCMP_EQUAL("Hello World\n", testBuffer.data());
}








TEST(_SysMemCheckpoint, EvenHeap) {
	_SysMemState s1;
	_SysMemState s2;
	_SysMemState s3;
	_SysMemState s4;

	_SysMemCheckpoint(&s1);
	s2 = s1;
	CHECK_EQUAL(0,_SysMemDifference(&s3, &s1, &s2));
	CHECK_EQUAL(0,_SysMemDifference(&s4, &s1, &s2));
	CHECK_EQUAL(0,_SysMemDifference(&s4, &s2, &s1));
	CHECK_EQUAL(s3.freeHeapSizeNow, s4.freeHeapSizeNow);
	CHECK_EQUAL(s3.freeHeapSizeMin, s4.freeHeapSizeMin);
}


TEST(_SysMemCheckpoint, OddHeap) {
	_SysMemState s1;
	_SysMemState s2;
	_SysMemState s3;
	_SysMemState s4;

	_SysMemCheckpoint(&s1);
	s2 = s1;
	s2.freeHeapSizeNow += 1;

	CHECK_EQUAL(1,_SysMemDifference(&s3, &s1, &s2));
	CHECK_EQUAL(1,_SysMemDifference(&s4, &s2, &s1));
	CHECK_EQUAL(s3.freeHeapSizeNow, s4.freeHeapSizeNow);
	CHECK_EQUAL(s3.freeHeapSizeMin, s4.freeHeapSizeMin);

	s2.freeHeapSizeNow -= 2;
	CHECK_EQUAL(1,_SysMemDifference(&s3, &s1, &s2));
	CHECK_EQUAL(1,_SysMemDifference(&s4, &s2, &s1));
	CHECK_EQUAL(s3.freeHeapSizeNow, s4.freeHeapSizeNow);
	CHECK_EQUAL(s3.freeHeapSizeMin, s4.freeHeapSizeMin);
}

TEST(_SysMemCheckpoint, FullHeap) {
	_SysMemState s1;
	_SysMemState s2;
	_SysMemState s3;

	s1.freeHeapSizeNow = 0;
	s1.freeHeapSizeMin = 0;
	s2 = s1;

	CHECK_EQUAL(0,_SysMemDifference(&s3, &s1, &s2));
}



TEST(malloc, ZeroElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(0);
	POINTERS_EQUAL(ptr, nullptr);

	/* Zero size, no deallocation necessary */
	// free(ptr);
}

TEST(malloc, OneElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(1);
	CHECK( ptr != nullptr );

	free(ptr);
}

TEST(malloc, ManyElements){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(13);
	CHECK( ptr != nullptr );
	std::strcpy(ptr,"no leak now");

	free(ptr);
}


TEST(malloc, OutOfMemory){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(100000);
	POINTERS_EQUAL(ptr, nullptr);

	/* Insufficient memory, no deallocation necessary */
	// free(ptr);
}



TEST(realloc, ZeroElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(12);
	std::strcpy(ptr,"cool");
	char *nptr = (char *)std::realloc(ptr, 0);
	POINTERS_EQUAL(nptr, nullptr);

	std::free(ptr);
	/* Not required, zero size block */
	//free(nptr);
}


TEST(realloc, OneElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(12);
	std::strcpy(ptr,"cool");
	char *nptr = (char *)std::realloc(ptr, 1);
	CHECK( ptr != nptr );
	CHECK_EQUAL(nptr[0], 'c');

	/* Not required, realloc already moved and freed old location */
	//free(ptr);
	std::free(nptr);
}


TEST(realloc, ManyElements){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::malloc(12);
	strcpy(ptr,"cool");
	char *nptr = (char *)std::realloc(ptr, 13);
	CHECK( ptr != nptr );
	STRCMP_EQUAL("cool", nptr);

	/* Not required, realloc already moved and freed old location */
	//free(ptr);
	std::free(nptr);
}


TEST(realloc, NullPtr){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::realloc(NULL, 13);
	CHECK( ptr != nullptr );

	std::free(ptr);
}

TEST(realloc, NullPtr_ZeroElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::realloc(NULL, 0);
	POINTERS_EQUAL(ptr, nullptr);

	/* Not required, no memory allocated */
	//std::free(ptr);
	//std::free(nptr);
}


TEST(realloc, OutOfMemory){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::realloc(NULL, 100000);
	POINTERS_EQUAL(ptr, nullptr);

	/* Insufficient memory, no deallocation necessary */
	// std::free(ptr);
}



TEST(calloc, ZeroElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::calloc(0, 0);
	POINTERS_EQUAL(ptr, nullptr);

	ptr = (char *)std::calloc(0, 1);
	POINTERS_EQUAL(ptr, nullptr);

	ptr = (char *)std::calloc(1, 0);
	POINTERS_EQUAL(ptr, nullptr);

	/* Not required, no memory allocated */
	//std::free(ptr);
}


TEST(calloc, OneElement){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::calloc(1, 1);
	CHECK( ptr != nullptr );
	CHECK_EQUAL(0, ptr[0]);

	std::free(ptr);
}


TEST(calloc, ManyElements){
	SysCheckMemory check = _SysCheckMemory();

	char val[20] = {};
	char *ptr = (char *)std::calloc(1,12);
	CHECK( ptr != nullptr );
	CHECK_EQUAL(0, memcmp(val, ptr, 12));

	char *nptr = (char *)std::calloc(5,4);
	CHECK( nptr != nullptr );
	CHECK_EQUAL(0, memcmp(val, nptr, 20));

	std::free(ptr);
	std::free(nptr);
}

TEST(calloc, OutOfMemory){
	SysCheckMemory check = _SysCheckMemory();

	char *ptr = (char *)std::calloc(1, 100000);
	POINTERS_EQUAL(ptr, nullptr);

	/* Insufficient memory, no deallocation necessary */
	// std::free(ptr);
}






TEST(fopen, ReadFlags)
{
	errno = 0;
	FILE *handle = std::fopen(Device.storage, "r");
	CHECK( handle != nullptr );
	CHECK_EQUAL(errno, 0);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);

	handle = std::fopen(Device.storage, "r+");
	CHECK( handle != nullptr );
	CHECK_EQUAL(errno, 0);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);

	handle = std::fopen(Device.storage, "rb");
	CHECK( handle != nullptr );
	CHECK_EQUAL(errno, 0);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);

	handle = std::fopen(Device.storage, "rb+");
	CHECK( handle != nullptr );
	CHECK_EQUAL(errno, 0);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);

	handle = std::fopen(Device.storage, "r+b");
	CHECK( handle != nullptr );
	CHECK_EQUAL(errno, 0);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}


TEST(fopen, WriteFlags)
{
	errno = 0;
	FILE *handle = std::fopen(Device.storage, "w");
	CHECK( handle != nullptr );
	CHECK_EQUAL(errno, 0);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);

	handle = std::fopen(Device.storage, "w+");
	CHECK( handle != nullptr );
	CHECK_EQUAL(errno, 0);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);

	handle = std::fopen(Device.storage, "wb");
	CHECK( handle != nullptr );
	CHECK_EQUAL(errno, 0);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);

	handle = std::fopen(Device.storage, "wb+");
	CHECK( handle != nullptr );
	CHECK_EQUAL(errno, 0);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);

	handle = std::fopen(Device.storage, "w+b");
	CHECK( handle != nullptr );
	CHECK_EQUAL(errno, 0);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}


TEST(fopen, AppendFlags)
{
	/*
	 * lseek() syscall is not implemented at this time, therefore "append" functionality
	 * is not supported.
	 */
	errno = 0;
	FILE *handle = std::fopen(Device.storage, "a");
	CHECK( handle != nullptr );
	CHECK_EQUAL(errno, EINVAL);

	errno = 0;
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}


TEST(fopen, InvalidFlags)
{
	errno = 0;
	FILE *handle = std::fopen(Device.storage, "R");
	POINTERS_EQUAL(handle, nullptr);
	CHECK_EQUAL(errno, EINVAL);

	errno = 0;
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}



TEST(fopen, AlreadyOpen)
{
	errno = 0;
	FILE *handle = std::fopen(Device.storage, "r");
	CHECK( handle != nullptr );
	CHECK_EQUAL(errno, 0);

	FILE *handle2 = std::fopen(Device.storage, "r");
	POINTERS_EQUAL(handle2, nullptr);
	CHECK_EQUAL(errno, EACCES);

	errno = 0;
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}


TEST(fopen, AlreadyClosed)
{
	errno = 0;
	FILE *handle = std::fopen(Device.storage, "r");
	CHECK( handle != nullptr );

	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}


/***************************************************************************************/

TEST(fwrite, ZeroLength)
{
	uint8_t data[] = {1, 2, 3, 4, 5};
	FILE *handle = std::fopen(Device.storage, "w");
	CHECK( handle != nullptr );

	CHECK_EQUAL(std::fwrite(data, sizeof(data[0]), 0, handle), 0u);

	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}

TEST(fwrite, OneByte)
{
	uint8_t data[] = {1, 2, 3, 4, 5};
	FILE *handle = std::fopen(Device.storage, "w");
	CHECK( handle != nullptr );

	CHECK_EQUAL(std::fwrite(data, sizeof(data[0]), 1, handle), 1u);

	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}


TEST(fwrite, ManyBytes)
{
	uint8_t data[] = {1, 2, 3, 4, 5};
	FILE *handle = std::fopen(Device.storage, "w");
	CHECK( handle != nullptr );

	CHECK_EQUAL(std::fwrite(data, sizeof(data[0]), 5, handle), 5u);

	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}


TEST(fwrite, ManyInts)
{
	uint32_t data[] = {1, 2, 3, 4, 5};
	FILE *handle = std::fopen(Device.storage, "w");
	CHECK( handle != nullptr );

	CHECK_EQUAL(std::fwrite(data, sizeof(data[0]), 5, handle), 5u);

	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}

TEST(fwrite, ToLarge)
{
	uint8_t data[] = {1, 2, 3, 4, 5};
	FILE *handle = std::fopen(Device.storage, "w");
	CHECK( handle != nullptr );

	/* fwrite will fail, but the exact number returned is dependent on the size of the internal
	 * buffer. The value returned will likely be a multiple of BUFSIZE until storage was filled.
	 */
	CHECK ( std::fwrite(data, sizeof(data[0]), 10000, handle) > 5u);
	CHECK_EQUAL(std::ferror(handle), 1);
	CHECK_EQUAL(std::feof(handle), 0);
	CHECK_EQUAL(errno, ENOSPC);

	errno = 0;
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(std::ferror(handle), 0);
	CHECK_EQUAL(std::feof(handle), 0);
	CHECK_EQUAL(errno, 0);
}


/***************************************************************************************/

TEST(fread, ZeroLength)
{
	uint8_t data[5] = {};
	FILE *handle = std::fopen(Device.storage, "r");
	CHECK( handle != nullptr );

	CHECK_EQUAL(std::fread(data, sizeof(data[0]), 0, handle), 0u);
	CHECK_EQUAL(std::ferror(handle), 0);
	CHECK_EQUAL(errno, 0);

	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}


TEST(fread, OneByte)
{
	uint8_t test[] = {1, 2, 3, 4, 5};
	uint8_t data[5] = {};
	FILE *handle = std::fopen(Device.storage, "w");
	CHECK( handle != nullptr );
	CHECK_EQUAL(std::fwrite(test, sizeof(test[0]), 1, handle), 1u);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);

	handle = std::fopen(Device.storage, "r");
	CHECK( handle != nullptr );
	CHECK_EQUAL(std::fread(data, sizeof(data[0]), 1, handle), 1u);
	CHECK_EQUAL(std::ferror(handle), 0);
	CHECK_EQUAL(errno, 0);

	CHECK_EQUAL(std::memcmp(test, data, 1), 0);

	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}


TEST(fread, ManyBytes)
{
	uint8_t test[] = {1, 2, 3, 4, 5};
	uint8_t data[5] = {};
	FILE *handle = std::fopen(Device.storage, "w");
	CHECK( handle != nullptr );
	CHECK_EQUAL(std::fwrite(test, sizeof(test[0]), 5, handle), 5u);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);

	handle = std::fopen(Device.storage, "r");
	CHECK( handle != nullptr );
	CHECK_EQUAL(std::fread(data, sizeof(data[0]), 5, handle), 5u);
	CHECK_EQUAL(std::ferror(handle), 0);
	CHECK_EQUAL(errno, 0);

	CHECK_EQUAL(std::memcmp(test, data, 5), 0);

	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}


TEST(fread, ManyInts)
{
	uint32_t test[] = {1, 2, 3, 4, 5};
	uint32_t data[5] = {};
	FILE *handle = std::fopen(Device.storage, "w");
	CHECK( handle != nullptr );
	CHECK_EQUAL(std::fwrite(test, sizeof(test[0]), 5, handle), 5u);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);

	handle = std::fopen(Device.storage, "r");
	CHECK( handle != nullptr );
	CHECK_EQUAL(std::fread(data, sizeof(data[0]), 5, handle), 5u);
	CHECK_EQUAL(std::ferror(handle), 0);
	CHECK_EQUAL(errno, 0);

	CHECK_EQUAL(std::memcmp(test, data, 5), 0);

	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);
}


TEST(fread, MultiBuffer)
{
	/*
	 * To verify sequential access is guaranteed across multiple transfers,
	 * this test forces multiple buffer transfers to access the memory.
	 */
	constexpr size_t TEST_SIZE = (BUFSIZ * 2);
	for (uint32_t index = 0; index < TEST_SIZE; index++) {
		bigTest[index] = index;
	}

	FILE *handle = std::fopen(Device.storage, "w");
	CHECK( handle != nullptr );
	CHECK_EQUAL(std::fwrite(bigTest, sizeof(bigTest[0]), TEST_SIZE, handle), TEST_SIZE);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);

	handle = std::fopen(Device.storage, "r");
	CHECK( handle != nullptr );
	CHECK_EQUAL(std::fread(bigData, sizeof(bigData[0]), TEST_SIZE, handle), TEST_SIZE);
	CHECK_EQUAL(std::ferror(handle), 0);
	CHECK_EQUAL(std::feof(handle), 0);
	CHECK_EQUAL(errno, 0);

	CHECK_EQUAL(std::memcmp(bigTest, bigData, TEST_SIZE), 0);

	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(std::ferror(handle), 0);
	CHECK_EQUAL(std::feof(handle), 0);
	CHECK_EQUAL(errno, 0);
}

// TODO - need to re-implement/design based on limited memory available
#if 0
TEST(fread, ToLarge)
{
	FILE *handle = std::fopen(Device.storage, "r");
	CHECK( handle != nullptr );

	/* fread will fail, but the exact number returned is dependent on the size of the internal
	 * buffer. The value returned will likely be a multiple of BUFSIZ until storage was exhausted.
	 */
	CHECK(std::fread(bigData, sizeof(bigData[0]), sizeof(bigData), handle) > 5u);
	CHECK_EQUAL(std::ferror(handle), 0);
	CHECK_EQUAL(std::feof(handle), 1);
	CHECK_EQUAL(errno, 0);

	errno = 0;
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(std::ferror(handle), 0);
	CHECK_EQUAL(std::feof(handle), 0);
	CHECK_EQUAL(errno, 0);
}
#endif


/*****************************************************************************************
 * Section break
 */
TEST(crc, Verification)
{
	uint8_t test[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

	CHECK_EQUAL(crc_mpeg2(test, &test[9]), 0x0376E6E7u);
}


/*****************************************************************************************
 * Section break
 */

TEST(uConfig, Invalid)
{
	/* Initialize storage with invalid values */
	std::unique_ptr<UserConfig::Config_t> testValue = std::make_unique<UserConfig::Config_t>();
	std::memset(testValue.get(), 0x5A, UserConfig::TableSize);

	FILE *handle = std::fopen(Device.storage, "wb");
	CHECK_EQUAL(std::fwrite(testValue.get(), UserConfig::TableSize, 1, handle), 1u);
	CHECK_EQUAL(std::fflush(handle), 0);
	CHECK_EQUAL(std::ferror(handle), 0);
	CHECK_EQUAL(std::fclose(handle), 0);
	CHECK_EQUAL(errno, 0);

	/* Validate interface initializes to known state */
	UserConfig testConfig; /* Object instantiation emulates a power-on event */
	const UserConfig::Cloud_t &cloud = testConfig.GetCloudConfig();
	const UserConfig::Wifi_t &wifi = testConfig.GetWifiConfig();

	std::memset(bigTest, 0x00, sizeof(bigTest));
	CHECK_EQUAL(std::memcmp(bigTest, &cloud, sizeof(UserConfig::Cloud_t)), 0);
	CHECK_EQUAL(std::memcmp(bigTest, &wifi, sizeof(UserConfig::Wifi_t)), 0);
}


TEST(uConfig, SetValues)
{
	/**
	 * Normally the objects would be defined at compile time, but for test purposes runtime
	 * instantiation will be used. Runtime instantiation will emulate a power-on event.
	 */

	/* Initialize storage to a known state, invalid values. This forces the interface to set safe
	 * defaults that we will later change.
	 */
	{
		std::unique_ptr<UserConfig::Config_t> testValue = std::make_unique<UserConfig::Config_t>();
		std::memset(testValue.get(), 0x5A, UserConfig::TableSize);

		FILE *handle = std::fopen(Device.storage, "wb");
		CHECK_EQUAL(std::fwrite(testValue.get(), UserConfig::TableSize, 1, handle), 1u);
		CHECK_EQUAL(std::fflush(handle), 0);
		CHECK_EQUAL(std::ferror(handle), 0);
		CHECK_EQUAL(std::fclose(handle), 0);
		CHECK_EQUAL(errno, 0);
	}

	/* Create test values */
	{
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
			CHECK_EQUAL(testConfig->SetWifiOn(true), true);
			CHECK_EQUAL(testConfig->SetWifiPassword( &testPassword, pwdSize ), true);
			CHECK_EQUAL(testConfig->SetWifiSsid(  &testSsid, ssidSize ), true);
			CHECK_EQUAL(testConfig->SetCloudKey( std::move(testKey) ), true);
		}


		{
			std::unique_ptr<UserConfig> testConfig = std::make_unique<UserConfig>();
			const UserConfig::Wifi_t &wifi = testConfig->GetWifiConfig();
			CHECK_EQUAL(wifi.isWifiOn, true);
			STRCMP_EQUAL( wifi.password.value.data(), testPassword.data() );
			CHECK_EQUAL( wifi.password.size, pwdSize );
			STRCMP_EQUAL( wifi.ssid.value.data(), testSsid.data() );
			CHECK_EQUAL( wifi.ssid.size, ssidSize );
		}
	}

}

/*****************************************************************************************
 * Section break
 */



