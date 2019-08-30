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

#include "lps22hb.hpp"
#include "CppUTest/TestHarness.h"

/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/

/* Force allocation of heap resources before using in a test. This prevents false
 * detection of a memory leak due to allocating space for a semaphore when
 * the I2C bus arbitration is setup one time.
 */
static sensor::LPS22HB notUsed(I2C2_Bus);

/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/


TEST_GROUP(LPS22HB){};

TEST(LPS22HB, available)
{
	sensor::LPS22HB lps22hb(I2C2_Bus);

	/* Sensors internally update data values at a periodic rate, so this test will:
	 *   1. Read data and discard
	 *   2. Verify no data is available
	 *   3. Wait for data to become available
	 *   4. Verify data is available
	 *   4. Read data and discard
	 *   4. Exit
	 */

	/* Synchronize test to sensor update rate */
	lps22hb.getPressure();
	lps22hb.getTemperature();
	while ( lps22hb.available() == 0 ){};

	CHECK( lps22hb.available() == 1 );
	lps22hb.getPressure();
	lps22hb.getTemperature();
	CHECK( lps22hb.available() == 0 );
}


TEST(LPS22HB, connected)
{
	sensor::LPS22HB lps22hb(I2C2_Bus);

	CHECK( lps22hb == true);
	CHECK( lps22hb.connected() == true);
}


TEST(LPS22HB, getTemperature)
{
	/* Assumption, test is being performed in an office setting with a nominal temperature
	 * and local board heating, range of 66F to 95F (18C to 35C) */

	sensor::LPS22HB lps22hb(I2C2_Bus);

	while ( lps22hb.available() == 0 ){};

	CHECK( lps22hb.getTemperature() > 18 );
	CHECK( lps22hb.getTemperature() < 35 );
}


TEST(LPS22HB, getPressure)
{
	/* Assumption, test is being performed in an office setting near sea level:
	 *  - At sea level nominal 1013.25 hPa
	 *  Test range sea level to 500 meters (~1500 feet) above sea level:
	 *  Reference: https://www.britannica.com/science/atmospheric-pressure
	 */
	sensor::LPS22HB lps22hb(I2C2_Bus);

	while ( lps22hb.available() == 0 ){};

	CHECK( lps22hb.getPressure()  > 1000 );
	CHECK( lps22hb.getPressure() < 1020 );
}
