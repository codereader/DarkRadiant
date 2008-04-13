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

#if !defined(INCLUDED_STREAM_TEXTSTREAM_H)
#define INCLUDED_STREAM_TEXTSTREAM_H

/// \file
/// \brief Text-output-formatting.

#include "itextstream.h"

#include <cctype>
#include <cstddef>
#include <cmath>
#include <stdio.h>
#include <string.h>
#include <algorithm>

#include "generic/arrayrange.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

typedef unsigned int Unsigned;

class FloatFormat
{
public:
  double m_f;
  int m_width;
  int m_precision;
  FloatFormat(double f, int width, int precision)
    : m_f(f), m_width(width), m_precision(precision)
  {
  }

	// Operator cast to std::string, useful for stream insertion
  operator std::string() const {
		static char buf[50];
		snprintf(buf, 50, "%*.*lf", m_width, m_precision, m_f);
		return std::string(buf);
	}
};

/// \brief Writes a floating point value to \p ostream with a specific width and precision.
inline std::ostream& operator<<(std::ostream& ostream, const FloatFormat& formatted) {
	ostream << std::string(formatted);
	return ostream;
}

/// \brief A wrapper for a TextInputStream optimised for reading a single character at a time.
template<typename TextInputStreamType, int SIZE = 1024>
class SingleCharacterInputStream
{
  TextInputStreamType& m_inputStream;
  char m_buffer[SIZE];
  char* m_cur;
  char* m_end;

  bool fillBuffer()
  {
    m_end = m_buffer + m_inputStream.read(m_buffer, SIZE);
    m_cur = m_buffer;
    return m_cur != m_end;
  }
public:

  SingleCharacterInputStream(TextInputStreamType& inputStream) : m_inputStream(inputStream), m_cur(m_buffer), m_end(m_buffer)
  {
  }
  bool readChar(char& c)
  {
    if(m_cur == m_end && !fillBuffer())
    {
      return false;
    }

    c = *m_cur++;
    return true;
  }
};

#endif
