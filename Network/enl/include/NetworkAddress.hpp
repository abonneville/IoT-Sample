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



#ifndef INCLUDE_NETWORKADDRESS_HPP_
#define INCLUDE_NETWORKADDRESS_HPP_


#include<array>

#include<stdint.h>
#include<cstring>

namespace enl
{


/**
 * NetworkAddress has been implemented using a byte array (std::array) to work with, IPv4, IPv6,
 * MAC addresses, etc... The byte array is consistent with the POSIX interface (sa_data[],
 * socket address). However, AWS secure socket interface uses a uint32_t to hold the IPv4 address,
 * as such, this interface has been modified to transfer between byte array and uint32_t.
 */
template<size_t addrSize>
class NetworkAddress
{
public:


	/**
	 * Constructors/initializers
	 */
	NetworkAddress() : address{} {}

	explicit NetworkAddress(const uint8_t * addr)
	{
		std::memcpy(address.data(), addr, addrSize );
	}

	template< typename T, /* force dependency on constructor parameter, not class */
    	typename  = typename std::enable_if< addrSize == 4 && std::is_same<T, uint32_t>::value >::type>
	explicit NetworkAddress(T addr)
	{
		address[3] = (addr >> 24);
		address[2] = (addr >> 16);
		address[1] = (addr >>  8);
		address[0] = (addr >>  0);
	}

	template <size_t Tsize = addrSize, /* force dependency on constructor/operator parameter, not class */
			typename = typename std::enable_if< Tsize == 4 >::type>
	NetworkAddress( uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet )
	{
		address[0] = first_octet;
		address[1] = second_octet;
		address[2] = third_octet;
		address[3] = fourth_octet;
	}


	/**
	 * Conversion operators
	 */
	template <size_t Tsize = addrSize, /* force dependency on constructor/operator parameter, not class */
			typename = typename std::enable_if< Tsize == 4 >::type>
	operator uint32_t() const
	{
		uint32_t retval = 0;
		retval  = (address[3] << 24);
		retval |= (address[2] << 16);
		retval |= (address[1] <<  8);
		retval |= (address[0] <<  0);

		return retval;
	}


	/**
	 * Assignment operators.
	 */
	NetworkAddress & operator=(uint8_t * addr)
	{
		std::memcpy(address.data(), addr, addrSize );
		return *this;
	}

	template< typename T, /* force dependency on constructor parameter, not class */
    	typename = typename std::enable_if< addrSize == 4 &&
										   ( (std::is_same<T, uint32_t>::value)
										  || (std::is_same<T, unsigned int>::value) )>::type>
	NetworkAddress & operator=(T addr)
	{
		address[3] = (addr >> 24);
		address[2] = (addr >> 16);
		address[1] = (addr >>  8);
		address[0] = (addr >>  0);

		return *this;
	}


	/**
	 * Mix bag of additional helpers
	 */
	using size_type = typename std::array<uint8_t, addrSize>::size_type;

	uint8_t & operator[]( size_type i) { return address[i]; }
	const uint8_t & operator[]( size_type i) const { return address[i]; }

	size_type size() const { return address.size(); }

	void copy(uint8_t *dest)
	{
		std::memcpy(dest, address.data(), addrSize);
	}

	uint8_t * data()
	{
		return address.data();
	}


	/**
	 * Logical operators
	 */
	bool operator==(const NetworkAddress &addr) const { return address == addr.address; };
	bool operator!=(const NetworkAddress &addr) const { return address != addr.address; };
	bool operator==(const uint8_t* addr) const
			{ return memcmp(addr, address.data(), addrSize) == 0; };
	bool operator!=(const uint8_t* addr) const
			{ return memcmp(addr, address.data(), addrSize) != 0; };

	/**
	 * Methods to display or format the address value
	 */
	/**
	 * Newlib (cygwin) does not yet support std::to_string, there are some hacks if needed but
	 * not compliant.
	 * https://stackoverflow.com/questions/12975341/to-string-is-not-a-member-of-std-says-g-mingw
	 */

private:
	std::array<uint8_t, addrSize> address;
};


using IPAddress = NetworkAddress<4>;

const IPAddress IP_None;
const IPAddress IP_Any(0, 0, 0, 0);
const IPAddress IP_LocalHost(127, 0, 0, 1);
const IPAddress IP_Broadcast(255, 255, 255, 255);

using MACAddress = NetworkAddress<6>;


} /* namespace enl */





#endif /* INCLUDE_NETWORKADDRESS_HPP_ */
