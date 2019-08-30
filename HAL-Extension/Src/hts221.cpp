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


/* Typedef -----------------------------------------------------------*/
enum class HTS221_Register: uint8_t
{
	WHO_AM_I          = 0x0F,
	AV_CONF           = 0x10,
	CTRL_REG1         = 0x20,
	CTRL_REG2         = 0x21,
	CTRL_REG3         = 0x22,
	STATUS_REG        = 0x27,
	HUMIDITY_OUT_L    = 0x28,
	HUMIDITY_OUT_H    = 0x29,
	TEMP_OUT_L        = 0x2A,
	TEMP_OUT_H        = 0x2B,

	CALIB_H0_RH_X2    = 0x30,
	CALIB_H1_RH_X2    = 0x31,
	CALIB_T0_DEGC_X8  = 0x32,
	CALIB_T1_DEGC_X8  = 0x33,
	CALIB_T1_T0_MSB   = 0x35,
	CALIB_H0_T0_OUT_L = 0x36,
	CALIB_H0_T0_OUT_H = 0x37,
	CALIB_H1_T0_OUT_L = 0x3A,
	CALIB_H1_T0_OUT_H = 0x3B,
	CALIB_T0_OUT_L    = 0x3C,
	CALIB_T0_OUT_H    = 0X3D,
	CALIB_T1_OUT_L    = 0x3E,
	CALIB_T1_OUT_H    = 0x3F
};


enum class AV_CONF : uint8_t
{
    RESERVED_BIT_MASK   = 0xC0,

    AVERAGE_HUMIDITY_4_SAMPLES   = 0b000,
    AVERAGE_HUMIDITY_8_SAMPLES   = 0b001,
    AVERAGE_HUMIDITY_16_SAMPLES  = 0b010,
    AVERAGE_HUMIDITY_32_SAMPLES  = 0b011,
    AVERAGE_HUMIDITY_64_SAMPLES  = 0b100,
    AVERAGE_HUMIDITY_128_SAMPLES = 0b101,
    AVERAGE_HUMIDITY_256_SAMPLES = 0b110,
    AVERAGE_HUMIDITY_512_SAMPLES = 0b111,

    AVERAGE_TEMPERATURE_2_SAMPLES   = (0b000 << 3),
    AVERAGE_TEMPERATURE_4_SAMPLES   = (0b001 << 3),
    AVERAGE_TEMPERATURE_8_SAMPLES   = (0b010 << 3),
    AVERAGE_TEMPERATURE_16_SAMPLES  = (0b011 << 3),
    AVERAGE_TEMPERATURE_32_SAMPLES  = (0b100 << 3),
    AVERAGE_TEMPERATURE_64_SAMPLES  = (0b101 << 3),
    AVERAGE_TEMPERATURE_128_SAMPLES = (0b110 << 3),
    AVERAGE_TEMPERATURE_256_SAMPLES = (0b111 << 3)

};


enum class CTRL_REG1 : uint8_t
{
	RESERVED_BIT_MASK = 0x78,

    POWER_DOWN = (0b0 << 7),
    POWER_UP   = (0b1 << 7),

    BDU_DISABLE = (0b0 << 2),
    BDU_ENABLE  = (0b1 << 2),

	DATARATE_ONE_SHOT = 0b00,
    DATARATE_1_HZ     = 0b01,
    DATARATE_7_HZ     = 0b10,
    DATARATE_12_5_HZ  = 0b11,
};


enum class CTRL_REG2 : uint8_t
{
	RESERVED_BIT_MASK = 0x7C,

	REBOOT_MEMORY = (0b0 << 7),

    HEATER_DISABLE = (0b0 << 1),
    HEATER_ENABLE  = (0b1 << 1),

	TRIG_ONE_SHOT = 0b1,
};


enum class STATUS_REG : uint8_t
{
	RESERVED_BIT_MASK = 0xFC,

	HUMIDITY_AVAIL = (0b1 << 1),
	TEMP_AVAIL = 0b1
};


typedef struct
{
	/* Device Table */
	uint8_t H0_rH_x2;
	uint8_t H1_rH_x2;
	uint8_t T0_degC_x8;
	uint8_t T1_degC_x8;
	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t T1_T0_msb;

	int16_t H0_T0_OUT;
	int16_t reserved3;
	int16_t H1_T0_OUT;
	int16_t T0_OUT;
	int16_t T1_OUT;

	/*Derived from above table */
	int16_t T0_degC;
	int16_t T1_degC;

}CalibrationTable_t;


/* Define ------------------------------------------------------------*/
static constexpr uint8_t devReadAddr = 0xBF;
static constexpr uint8_t devWriteAddr = 0xBE;
static constexpr uint8_t devId = 0xBC;

/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/

/* Function prototypes -----------------------------------------------*/

/* External functions ------------------------------------------------*/

namespace sensor
{



class HTS221::impl
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
	 * @param reg is the HTS221 register to be accessed
	 * @retval byte value from register
	 */
	uint8_t readByte( HTS221_Register reg )
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
	 * @brief Reads a signed 16-bit value from the requested register location
	 * @param reg is the HTS221 register to be accessed
	 * @retval signed 16-bit value from register
	 */
	int16_t read16bit( HTS221_Register reg )
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
	 * @param reg is the HTS221 register to be accessed
	 * @param value to be written into register
	 */
	void writeByte( HTS221_Register reg, uint8_t value )
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
		/**
		 * Note: contrary to the HTS221 datasheet, the device does not fully support auto
		 * incrementing the register address during block transfers of data. The following
		 * are known limitations:
		 * 		- starting a transfer before address 0x30, will not carry over to register
		 * 		address 0x30 and above. Registers between 0x00 to 0x29 appear to handle block
		 * 		transfer.
		 * 		- starting a transfer before address 0x35, will result in register 0x35
		 * 		reporting invalid values. Even if the transfer starts at 0x30, register 0x35
		 * 		will be invalid.
		 *
		 * 	Recommendation: when accessing calibration data, perform a partial or full (L/H)
		 * 	register access only, do not attempt block transfer for calibration data.
		 */
		/* Temperature coefficients - vertical axis */
		calTable.T1_T0_msb = readByte(HTS221_Register::CALIB_T1_T0_MSB);

		calTable.T0_degC_x8 = readByte(HTS221_Register::CALIB_T0_DEGC_X8);
		calTable.T1_degC_x8 = readByte(HTS221_Register::CALIB_T1_DEGC_X8);

		calTable.T0_degC = (((int16_t)(calTable.T1_T0_msb & 0x03)) << 8) | ((int16_t)calTable.T0_degC_x8);
		calTable.T1_degC = (((int16_t)(calTable.T1_T0_msb & 0x0C)) << 6) | ((int16_t)calTable.T1_degC_x8);

		/* Temperature coefficients - horizontal axis */
		calTable.T0_OUT = read16bit(HTS221_Register::CALIB_T0_OUT_L);
		calTable.T1_OUT = read16bit(HTS221_Register::CALIB_T1_OUT_L);


		/* Humidity coefficients - vertical axis */
		calTable.H0_rH_x2 = readByte(HTS221_Register::CALIB_H0_RH_X2);
		calTable.H1_rH_x2 = readByte(HTS221_Register::CALIB_H1_RH_X2);

		/* Humidity coefficients - horizontal axis */
		calTable.H0_T0_OUT = read16bit(HTS221_Register::CALIB_H0_T0_OUT_L);
		calTable.H1_T0_OUT = read16bit(HTS221_Register::CALIB_H1_T0_OUT_L);
	}


	/**
	 * @brief Convert raw ADC value into a temperature value
	 * @param value is the raw ADC value to be converted
	 * @retval range -40 to 120 deg C
	 */
	int16_t calcTemperature(int16_t T_in)
	{
		/* Linear interpolation */
		int32_t tmp32 = ((int32_t)(T_in - calTable.T0_OUT)) * ((int32_t)(calTable.T1_degC - calTable.T0_degC) );
		int16_t T_out = tmp32 / (calTable.T1_OUT - calTable.T0_OUT) + calTable.T0_degC;

		/* Remove x8 embedded in T_degC, truncate result */
		T_out = T_out >> 3;

		return T_out;
	}


	/**
	 * @brief Convert raw ADC value into a humidity value
	 * @param value is the humidity value obtained from the ADC
	 * @retval range 0 to 100 rH %
	 */
	uint16_t calcHumidity(int16_t H_in)
	{
		/* Linear interpolation */
		int32_t tmp = ((int32_t)(H_in - calTable.H0_T0_OUT)) * ((int32_t)(calTable.H1_rH_x2 - calTable.H0_rH_x2) );
		int16_t H_T_out = (tmp/(calTable.H1_T0_OUT - calTable.H0_T0_OUT) + calTable.H0_rH_x2 );

		/* Remove x2 embedded in H_rH_x2, truncate result */
		H_T_out = H_T_out >> 1;

		/* Saturation condition*/
		H_T_out = H_T_out > 100 ? 100 : H_T_out;
		H_T_out = H_T_out < 0    ? 0  : H_T_out;

		return H_T_out;
	}

	BA_I2C_descriptor_t bus;
	CalibrationTable_t calTable;
};


HTS221::~HTS221() {}


/**
 * @brief Setup and initialize sensor, upon completion sensor is fully operational
 * @param location is the physical I2C bus the device is attached to
 */
HTS221::HTS221(BA_I2C_descriptor_t location) :
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
		/* Set filters for temperature and humidity */
		value  = pimpl->readByte( HTS221_Register::AV_CONF);
		value &= (uint8_t)AV_CONF::RESERVED_BIT_MASK;
		value |= (uint8_t)AV_CONF::AVERAGE_HUMIDITY_32_SAMPLES;
		value |= (uint8_t)AV_CONF::AVERAGE_TEMPERATURE_16_SAMPLES;
		pimpl->writeByte( HTS221_Register::AV_CONF, value );

		/* Disable internal heater */
		value = pimpl->readByte(HTS221_Register::CTRL_REG2);
		value &= (uint8_t)CTRL_REG2::RESERVED_BIT_MASK;
		value |= (uint8_t)CTRL_REG2::HEATER_DISABLE;
		pimpl->writeByte( HTS221_Register::CTRL_REG2, value );

		/* Set update rate and mode, activate device*/
		value = pimpl->readByte(HTS221_Register::CTRL_REG1);
		value &= (uint8_t)CTRL_REG1::RESERVED_BIT_MASK;
		value |= (uint8_t)CTRL_REG1::BDU_ENABLE;
		value |= (uint8_t)CTRL_REG1::DATARATE_1_HZ;
		value |= (uint8_t)CTRL_REG1::POWER_UP;
		pimpl->writeByte( HTS221_Register::CTRL_REG1, value );
	}
}


/**
 * @brief Returns the most current temperature value
 * @retval temperature value in Celsius, -40 to 120 C
 */
int16_t HTS221::getTemperature()
{
	int16_t value = pimpl->read16bit(HTS221_Register::TEMP_OUT_L);

	return pimpl->calcTemperature(value);;
}


/**
 * @brief Returns the most current humidity value
 * @retval humidity value in rH, 0 to 100 % rH
 */
uint16_t HTS221::getHumidity()
{
	int16_t value = pimpl->read16bit(HTS221_Register::HUMIDITY_OUT_L);

	return pimpl->calcHumidity(value);
}


/**
 * @brief  Indicates if there is one or more sensor values available to be read
 * @retval 1 if data available, 0 if not.
 *
 */
int32_t HTS221::available()
{
	uint8_t value = pimpl->readByte(HTS221_Register::STATUS_REG);
	value &= ( ~(uint8_t)STATUS_REG::RESERVED_BIT_MASK );
	return (value != 0) ? 1: 0;

}

/**
 * @brief Evaluates if the object has established contact with sensor
 * @retval true indicates sensor is connected, false otherwise
 */
HTS221::operator bool()
{
	return connected();
}

/**
 * @brief Reports back if the sensor is accessible
 * @retval true indicates sensor is connected, false otherwise
 */
bool HTS221::connected()
{
	uint8_t value = pimpl->readByte( HTS221_Register::WHO_AM_I );

	return ( value == devId );
}

} /* namespace sensor */
