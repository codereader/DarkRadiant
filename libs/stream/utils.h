#pragma once

// Fixed width integer types are defined in the C++11 header <cstdint>
#include <cstdint>

#include "idatastream.h"
#include <ostream>
#include <algorithm>

namespace stream
{

/**
 * greebo: Writes the given number value to the given output stream in Big Endian byte order,
 * regardless of the calling platform's endianness.
 */
template<typename ValueType>
void writeBigEndian(std::ostream& stream, ValueType value)
{
	ValueType output = value;

#ifndef __BIG_ENDIAN__
	std::reverse(reinterpret_cast<char*>(&output), reinterpret_cast<char*>(&output) + sizeof(ValueType));
#endif

	stream.write(reinterpret_cast<const char*>(&output), sizeof(ValueType));
}

/**
* greebo: Writes the given number value to the given output stream in Little Endian byte order,
* regardless of the calling platform's endianness.
*/
template<typename ValueType>
void writeLittleEndian(std::ostream& stream, ValueType value)
{
	ValueType output = value;

#ifdef __BIG_ENDIAN__
	std::reverse(reinterpret_cast<char*>(&output), reinterpret_cast<char*>(&output) + sizeof(ValueType));
#endif

	stream.write(reinterpret_cast<const char*>(&output), sizeof(ValueType));
}

/**
 * greebo: Read an integer type stored in little endian format 
 * from the given stream and returns its value.
 */
template<typename ValueType>
inline ValueType readLittleEndian(InputStream& stream)
{
	ValueType value;
	stream.read(reinterpret_cast<InputStream::byte_type*>(&value), sizeof(ValueType));

#ifdef __BIG_ENDIAN__
	std::reverse(reinterpret_cast<InputStream::byte_type*>(&value), reinterpret_cast<InputStream::byte_type*>(&value) + sizeof(ValueType));
#endif

	return value;
}

inline void readByte(InputStream& stream, InputStream::byte_type& value)
{
	stream.read(&value, 1);
}

inline InputStream::byte_type readByte(InputStream& stream)
{
	InputStream::byte_type value;
	stream.read(&value, 1);

	return value;
}

}
