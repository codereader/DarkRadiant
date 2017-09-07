/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

#if defined(__GNUC__)

#define	_ISOC9X_SOURCE	1
#define _ISOC99_SOURCE	1

#define	__USE_ISOC9X	1
#define	__USE_ISOC99	1

#include <stdint.h>

#endif

#include "idatastream.h"
#include <ostream>
#include <algorithm>

// if C99 is unavailable, fall back to the types most likely to be the right sizes
#if !defined(int16_t)
typedef signed short int16_t;
#endif
#if !defined(uint16_t)
typedef unsigned short uint16_t;
#endif
#if !defined(int32_t)
typedef signed int int32_t;
#endif
#if !defined(uint32_t)
typedef unsigned int uint32_t;
#endif


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

template<typename InputStreamType, typename Type>
inline void istream_read_little_endian(InputStreamType& istream, Type& value)
{
  istream.read(reinterpret_cast<typename InputStreamType::byte_type*>(&value), sizeof(Type));
#if defined(__BIG_ENDIAN__)
  std::reverse(reinterpret_cast<typename InputStreamType::byte_type*>(&value), reinterpret_cast<typename InputStreamType::byte_type*>(&value) + sizeof(Type));
#endif
}

template<typename InputStreamType, typename Type>
inline void istream_read_big_endian(InputStreamType& istream, Type& value)
{
  istream.read(reinterpret_cast<typename InputStreamType::byte_type*>(&value), sizeof(Type));
#if !defined(__BIG_ENDIAN__)
  std::reverse(reinterpret_cast<typename InputStreamType::byte_type*>(&value), reinterpret_cast<typename InputStreamType::byte_type*>(&value) + sizeof(Type));
#endif
}

template<typename InputStreamType>
inline void istream_read_byte(InputStreamType& istream, typename InputStreamType::byte_type& b)
{
  istream.read(&b, 1);
}

template<typename InputStreamType>
inline typename InputStreamType::byte_type istream_read_byte(InputStreamType& istream)
{
  typename InputStreamType::byte_type b;
  istream.read(&b, sizeof(typename InputStreamType::byte_type));
  return b;
}
