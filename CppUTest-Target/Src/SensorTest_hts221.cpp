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

#include "hts221.hpp"
#include "CppUTest/TestHarness.h"



/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/

/* Force allocation of heap resources before using in a test. This prevents false
 * detection of a memory leak due to allocating space for a semaphore when
 * the I2C bus arbitration is setup one time.
 */
static sensor::HTS221 notUsed(I2C2_Bus);

/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/


TEST_GROUP(HTS221){};

TEST(HTS221, available)
{
	sensor::HTS221 hts221(I2C2_Bus);

	/* Sensors internally update data values at a periodic rate, so this test will:
	 *   1. Read data and discard
	 *   2. Verify no data is available
	 *   3. Wait for data to become available
	 *   4. Verify data is available
	 *   4. Read data and discard
	 *   4. Exit
	 */

	/* Synchronize test to sensor update rate */
	hts221.getHumidity();
	hts221.getTemperature();
	while ( hts221.available() == 0 ){};

	CHECK( hts221.available() == 1 );
	hts221.getHumidity();
	hts221.getTemperature();
	CHECK( hts221.available() == 0 );
}


TEST(HTS221, connected)
{
	sensor::HTS221 hts221(I2C2_Bus);

	CHECK( hts221 == true);
	CHECK( hts221.connected() == true);
}


TEST(HTS221, getTemperature)
{
	/* Assumption, test is being performed in an office setting with a nominal temperature
	 * and local board heating, range of 66F to 95F (18C to 35C) */

	sensor::HTS221 hts221(I2C2_Bus);

	while ( hts221.available() == 0 ){};

	CHECK( hts221.getTemperature() > 18 );
	CHECK( hts221.getTemperature() < 35 );
}


TEST(HTS221, getHumidity)
{
	/* Assumption, test is being performed in an office setting with a nominal humidity
	 * range of 30 to 65% rH  */

	sensor::HTS221 hts221(I2C2_Bus);

	while ( hts221.available() == 0 ){};

	CHECK( hts221.getHumidity() > 30u );
	CHECK( hts221.getHumidity() < 65u );
}
