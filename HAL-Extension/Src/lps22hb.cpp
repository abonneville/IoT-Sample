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


/* Typedef -----------------------------------------------------------*/
enum class LPS22HB_Register: uint8_t
{
	INTERRUPT_CFG     = 0x0B,
	THS_P_L			  = 0x0C,
	THS_P_H			  = 0x0D,
	WHO_AM_I          = 0x0F,
	CTRL_REG1         = 0x10,
	CTRL_REG2         = 0x11,
	CTRL_REG3         = 0x12,
	FIFO_CTRL		  = 0x14,
	REF_P_XL		  = 0x15,
	REF_P_L 		  = 0x16,
	REF_P_H 		  = 0x17,
	RPDS_L			  = 0x18,
	RPDS_H			  = 0x19,
	RES_CONF		  = 0x1A,
	INT_SOURCE		  = 0x25,
	FIFO_STATUS		  = 0x26,
	STATUS_REG        = 0x27,

	PRESS_OUT_XL      = 0x28,
	PRESS_OUT_L       = 0x29,
	PRESS_OUT_H       = 0x2A,
	TEMP_OUT_L        = 0x2B,
	TEMP_OUT_H        = 0x2C,

	LPFP_RES          = 0X33
};




enum class CTRL_REG1 : uint8_t
{
	RESERVED_BIT_MASK = 0x80,

	DATARATE_ONE_SHOT = (0b000 << 4),
	DATARATE_1_HZ     = (0b001 << 4),
	DATARATE_10_HZ    = (0b010 << 4),
	DATARATE_25_HZ    = (0b011 << 4),
	DATARATE_50_HZ    = (0b100 << 4),
	DATARATE_75_HZ    = (0b101 << 4),

	LPFP_DISABLED      = (0b00 << 2),
	LPFP_EN_BW_9HZ     = (0b10 << 2),
	LPFP_EN_BW_20HZ    = (0b11 << 2),

    BDU_DISABLE = (0b0 << 1),
    BDU_ENABLE  = (0b1 << 1),

	SIM_3_WIRE  = 0b1,
	SIM_4_WIRE  = 0b0,
};


enum class CTRL_REG2 : uint8_t
{
	RESERVED_BIT_MASK = 0x02,

	REBOOT_MEMORY = (0b1 << 7),

	FIFO_ENABLE    = (0b1 << 6),
	FIFO_DISABLE   = (0b0 << 6),

	STOP_ON_FTH_ENABLE    = (0b1 << 5),
	STOP_ON_FTH_DISABLE   = (0b0 << 5),

	IF_ADD_INC_ENABLE      = (0b1 << 4),
	IF_ADD_INC_DISABLE     = (0b0 << 4),

	I2C_ENABLE     = (0b0 << 3),
	I2C_DISABLE    = (0b1 << 3),

	SWRESET  = (0b1 << 2),

	TRIG_ONE_SHOT = 0b1,
};


enum class FIFO_CTRL : uint8_t
{
	RESERVED_BIT_MASK = 0x00,

	MODE_BYPASS           = (0b000 << 5),
	MODE_FIFO             = (0b001 << 5),
	MODE_STREAM           = (0b010 << 5),
	MODE_STREAM_TO_FIFO   = (0b011 << 5),
	MODE_BYPASS_TO_STREAM = (0b100 << 5),
	MODE_DYNAMIC_STREAM   = (0b110 << 5),
	MODE_BYPASS_TO_FIFO   = (0b111 << 5)
};

enum class RES_CONF : uint8_t
{
	RESERVED_BIT_MASK = 0xFE,

	LOW_CURRENT_ENABLE  = 0b1,
	LOW_CURRENT_DISABLE = 0b0,

};

enum class STATUS_REG : uint8_t
{
	RESERVED_BIT_MASK = 0xCC,

	TEMP_OVR     = (0b1 << 5),
	PRESS_OVR    = (0b1 << 4),

	TEMP_AVAIL  = (0b1 << 1),
	PRESS_AVAIL = (0b1 << 0)
};


typedef struct
{
	/* Device Table */
	int16_t RPDS;


	/*Derived from above table */

}CalibrationTable_t;


/* Define ------------------------------------------------------------*/
static constexpr uint8_t devReadAddr = 0xBB;
static constexpr uint8_t devWriteAddr = 0xBA;
static constexpr uint8_t devId = 0xB1;

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/

/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/

namespace sensor
{



class LPS22HB::impl
{
public:
	impl(BA_I2C_descriptor_t b) :
		bus(b)
	{
		BA_I2C_init(b);

		readCalibration();
	};


	/**
	 * @brief Reads a single byte from the requested register location
	 * @param reg is the LPS22HB register to be accessed
	 * @retval byte value from register
	 */
	uint8_t readByte( LPS22HB_Register reg )
	{
		uint8_t data = 0;

		BA_I2C_buffer_t buffer;
		buffer.outbound = (uint8_t *)&reg;
		buffer.outSize = 1;
		buffer.inbound = &data;
		buffer.inSize = 1;

		BA_I2C_read( bus, devReadAddr, &buffer );

		return data;
	}


	/**
	 * @brief Reads a block of data starting from the requested register location
	 * @param reg is the initial/base LPS22HB register to be accessed
	 * @param buf is the destination to store data
	 * @param size is how many bytes to be read/stored
	 */
	void readBlock( LPS22HB_Register reg, uint8_t *buf, size_t size )
	{
		BA_I2C_buffer_t buffer;
		buffer.outbound = (uint8_t *)&reg;
		buffer.outSize = 1;
		buffer.inbound = buf;
		buffer.inSize = size;

		BA_I2C_read( bus, devReadAddr, &buffer );
	}


	/**
	 * @brief Reads a signed 16-bit value from the requested register location
	 * @param reg is the LPS22HB register to be accessed
	 * @retval signed 16-bit value from register
	 */
	int16_t read16bit( LPS22HB_Register reg )
	{
		uint8_t data[2] {};

		uint8_t regAutoIncrement = (uint8_t)reg | 0x80;

		BA_I2C_buffer_t buffer;
		buffer.outbound = &regAutoIncrement;
		buffer.outSize = 1;
		buffer.inbound = data;
		buffer.inSize = 2;

		BA_I2C_read( bus, devReadAddr, &buffer );

		return (int16_t)( (int16_t)data[1] << 8 | data[0] );
	}


	/**
	 * @brief Writes a single byte into the requested register location
	 * @param reg is the LPS22HB register to be accessed
	 * @param value to be written into register
	 */
	void writeByte( LPS22HB_Register reg, uint8_t value )
	{
		uint8_t txBuffer[2] {};
		txBuffer[0] = (uint8_t)reg;
		txBuffer[1] = value;

		BA_I2C_buffer_t buffer;
		buffer.outbound = txBuffer;
		buffer.outSize = sizeof(txBuffer);
		buffer.inbound = nullptr;
		buffer.inSize = 0;

		BA_I2C_write( bus, devWriteAddr, &buffer );
	}


	/**
	 * @brief Reads the device's entire calibration table into local memory
	 */
	void readCalibration()
	{
		/* Pressure offset value, one-point calibration after soldering */
		calTable.RPDS = read16bit(LPS22HB_Register::RPDS_L);
	}


	BA_I2C_descriptor_t bus;
	CalibrationTable_t calTable;
};


LPS22HB::~LPS22HB() {}


/**
 * @brief Setup and initialize sensor, upon completion sensor is fully operational
 * @param location is the physical I2C bus the device is attached to
 */
LPS22HB::LPS22HB(BA_I2C_descriptor_t location) :
		pimpl{ std::make_unique<impl>(location) }
{
	uint8_t value = 0;

	/**
	 * At this point, both the I2C and DMA interface are already configured and operational
	 * from the HAL boot code (STM32CubeMX). Here we need to setup and configure the actual
	 * sensor for use by the application.
	 */

	/* Confirm sensor is present */
	if( connected() )
	{

		/* Disable internal heater */
		value = pimpl->readByte(LPS22HB_Register::CTRL_REG2);
		value &= (uint8_t)CTRL_REG2::RESERVED_BIT_MASK;
		value |= (uint8_t)CTRL_REG2::FIFO_DISABLE;
		value |= (uint8_t)CTRL_REG2::IF_ADD_INC_ENABLE;
		value |= (uint8_t)CTRL_REG2::I2C_ENABLE;
		pimpl->writeByte( LPS22HB_Register::CTRL_REG2, value );

		/* Set update rate and mode, activate device*/
		value = pimpl->readByte(LPS22HB_Register::CTRL_REG1);
		value &= (uint8_t)CTRL_REG1::RESERVED_BIT_MASK;
		value |= (uint8_t)CTRL_REG1::BDU_ENABLE;
		value |= (uint8_t)CTRL_REG1::DATARATE_1_HZ;
		value |= (uint8_t)CTRL_REG1::LPFP_EN_BW_9HZ;
		pimpl->writeByte( LPS22HB_Register::CTRL_REG1, value );
	}
}


/**
 * @brief Returns the most current temperature value
 * @retval temperature value in Celsius, -40 to 120 C
 */
int16_t LPS22HB::getTemperature()
{
	int16_t value = pimpl->read16bit(LPS22HB_Register::TEMP_OUT_L);

	/* Remove x100 scaler in temperature, truncate result */
	value = value / 100;

	return value;
}


/**
 * @brief Returns the current pressure value
 * @retval range 260 to 1260 hPa
 */
uint16_t LPS22HB::getPressure()
{
	int32_t press = 0;
	pimpl->readBlock(LPS22HB_Register::PRESS_OUT_XL, (uint8_t *)&press, 3);

	/* convert the 2's complement 24 bit to 2's complement 32 bit */
	if(press & 0x00800000)
		press |= 0xFF000000;

	press = press >> 12;

	press = (press < 260) ? 260 : press;
	press = (press > 1260 ) ? 1260 : press;

	return press;
}


/**
 * @brief  Indicates if there is one or more sensor values available to be read
 * @retval 1 if data available, 0 if not.
 *
 */
int32_t LPS22HB::available()
{
	uint8_t value = pimpl->readByte(LPS22HB_Register::STATUS_REG);
	value &= ( (uint8_t)STATUS_REG::PRESS_AVAIL | (uint8_t)STATUS_REG::TEMP_AVAIL );
	return (value != 0) ? 1: 0;

}

/**
 * @brief Evaluates if the object has established contact with sensor
 * @retval true indicates sensor is connected, false otherwise
 */
LPS22HB::operator bool()
{
	return connected();
}

/**
 * @brief Reports back if the sensor is accessible
 * @retval true indicates sensor is connected, false otherwise
 */
bool LPS22HB::connected()
{
	uint8_t value = pimpl->readByte( LPS22HB_Register::WHO_AM_I );

	return ( value == devId );
}

} /* namespace sensor */
