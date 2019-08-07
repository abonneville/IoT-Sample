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

#include "CppUTest/TestHarness.h"
#include "NetworkAddress.hpp"



/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/

/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/


TEST_GROUP(IPAddress) {};
TEST_GROUP(MACAddress) {};

TEST(IPAddress, uint32_t)
{

	uint32_t addr = 123456789;
	enl::IPAddress ip1 {addr};
	enl::IPAddress ip2 (addr);
	enl::IPAddress ip3( (uint32_t)0 );

	CHECK( ip1 == ip2 );
	CHECK( ip1 != ip3 );
	CHECK( ip1 == addr );
	CHECK( ip3 != addr );

	uint32_t addr2 = ip3;
	CHECK( addr2 == 0 );
	CHECK( ip3 == addr2 );

	ip3 = 9834u;
	ip3 = (uint32_t)76;
	CHECK( ip3[0] == 76 );
	CHECK( ip3[1] == 0 );
	CHECK( ip3[2] == 0 );
	CHECK( ip3[3] == 0 );
	addr = ip3;
	CHECK ( addr == 76 );
}


TEST(IPAddress, byteArray)
{
	uint8_t addr[4] = { 127, 0, 0, 1 };
	enl::IPAddress ip1 {addr};
	enl::IPAddress ip2(addr);
	enl::IPAddress ip3( (uint32_t)0 );
	enl::IPAddress ip4 {127, 0 , 0, 1};

	CHECK( ip1 == ip2 );
	CHECK( ip1 != ip3 );
	CHECK( ip1 == ip4 );

	CHECK( ip1 == addr );
	CHECK( ip3 != addr );

	uint32_t addr2 = ip3;
	CHECK( addr2 == 0 );
	CHECK( ip3 == addr2 );

	ip4 = {234, 76, 0, 1};
	CHECK( ip4[0] == 234 );
	CHECK( ip4[1] == 76 );
	CHECK( ip4[2] == 0 );
	CHECK( ip4[3] == 1 );

	ip4[2] = 44;
	CHECK( ip4[0] == 234 );
	CHECK( ip4[1] == 76 );
	CHECK( ip4[2] == 44 );
	CHECK( ip4[3] == 1 );

	/* Verify access to raw pointer */
	CHECK( ip4.data() == &ip4[0] );
}


TEST(IPAddress, otherOperators)
{

	/* size() */
	enl::IPAddress ip1;
	CHECK( 4 == ip1.size() );

	/* copy() */
	uint8_t addr[4] {};
	ip1 = {127, 0, 0, 1 };

	ip1.copy( addr );
	CHECK( ip1 == addr );
	CHECK( 127 == addr[0] );
	CHECK( 0   == addr[1] );
	CHECK( 0   == addr[2] );
	CHECK( 1   == addr[3] );

	/* Constants */
	ip1 = enl::IP_Any;
	CHECK( ip1[0] == 0 );
	CHECK( ip1[1] == 0 );
	CHECK( ip1[2] == 0 );
	CHECK( ip1[3] == 0 );

	ip1 = enl::IP_LocalHost;
	CHECK( ip1[0] == 127 );
	CHECK( ip1[1] == 0 );
	CHECK( ip1[2] == 0 );
	CHECK( ip1[3] == 1 );

	ip1 = enl::IP_None;
	CHECK( ip1[0] == 0 );
	CHECK( ip1[1] == 0 );
	CHECK( ip1[2] == 0 );
	CHECK( ip1[3] == 0 );

	ip1 = enl::IP_Broadcast;
	CHECK( ip1[0] == 255 );
	CHECK( ip1[1] == 255 );
	CHECK( ip1[2] == 255 );
	CHECK( ip1[3] == 255 );

}



TEST(MACAddress, byteArray)
{
	uint8_t addr[6] = { 127, 0, 0, 1, 3, 89};
	enl::MACAddress mac1 {addr};
	enl::MACAddress mac2(addr);
	enl::MACAddress mac3;
	enl::MACAddress mac4 {addr};

	CHECK( mac1 == mac2 );
	CHECK( mac1 != mac3 );
	CHECK( mac1 == mac4 );

	CHECK( mac1 == addr );
	CHECK( mac3 != addr );

	uint8_t addr2[6] = {234, 76, 0, 1, 234, 255};
	mac4 =  addr2;
	CHECK( mac4[0] == 234 );
	CHECK( mac4[1] == 76 );
	CHECK( mac4[2] == 0 );
	CHECK( mac4[3] == 1 );
	CHECK( mac4[4] == 234 );
	CHECK( mac4[5] == 255 );

	mac4[2] = 44;
	CHECK( mac4[0] == 234 );
	CHECK( mac4[1] == 76 );
	CHECK( mac4[2] == 44 );
	CHECK( mac4[3] == 1 );
	CHECK( mac4[4] == 234 );
	CHECK( mac4[5] == 255 );
}

TEST(MACAddress, otherOperators)
{

	/* size() */
	enl::MACAddress mac1;
	CHECK( 6 == mac1.size() );

	/* copy() */
	uint8_t addr[6] {};
	uint8_t addr2[6] = {127, 0, 0, 1, 189, 255 };
	mac1 = addr2;

	mac1.copy( addr );
	CHECK( mac1 == addr );
	CHECK( 127 == addr[0] );
	CHECK( 0   == addr[1] );
	CHECK( 0   == addr[2] );
	CHECK( 1   == addr[3] );
	CHECK( 189 == addr[4] );
	CHECK( 255 == addr[5] );
}
