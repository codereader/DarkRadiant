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

#if !defined(INCLUDED_ITEXTSTREAM_H)
#define INCLUDED_ITEXTSTREAM_H

/// \file
/// \brief Text-stream interfaces.

#include <cstddef>
#include <string>
#include <streambuf>
#include <istream>
#include <cassert>
#include <sstream>
#include <iostream>

#include "generic/static.h"

/// \brief A read-only character-stream.
// OrbWeaver: merged functionality from TextStreambufAdaptor onto this class
// directly.
class TextInputStream
: public std::streambuf
{
    // Buffer to use for reading
    static const std::size_t BUFFER_SIZE = 8192;
    char _buffer[BUFFER_SIZE];

protected:
	
    /* Implementations of stream-specific virtual functions on std::streambuf */
    
    // Replenish the controlled buffer with characters from the underlying
    // input sequence.
    int underflow() {
        
        // Read next block of BUFFER_SIZE characters into the buffer from
        // the underlying TextInputStream.
        std::size_t charsRead = this->read(_buffer, BUFFER_SIZE);
        
        // Set up the internal pointers correctly
        assert(charsRead <= BUFFER_SIZE);
        std::streambuf::setg(_buffer, _buffer, _buffer + charsRead);
        
        // Return the next character, or EOF if there were no more characters
        if (charsRead > 0)
        	return static_cast<int>(_buffer[0]);
        else
        	return EOF;
    }
    
public:

	/// \brief Attempts to read the next \p length characters from the stream to \p buffer.
	/// Returns the number of characters actually stored in \p buffer.
	virtual std::size_t read(char* buffer, std::size_t length) = 0;
  
};

class NullOutputBuf :
	public std::streambuf
{
protected:
	virtual std::size_t xsputn(const char*, std::size_t len) {
		// Override the virtual xsputn method to do nothing instead
		return len;
	}
};

class NullOutputStream : 
	public std::ostream
{
	NullOutputBuf _nullBuf;
public:
	NullOutputStream() :
		std::ostream(&_nullBuf)
	{}
};

class OutputStreamHolder
{
  NullOutputStream m_nullOutputStream;
  std::ostream* m_outputStream;
public:
  OutputStreamHolder()
    : m_outputStream(&m_nullOutputStream)
  {
  }
  void setOutputStream(std::ostream& outputStream)
  {
    m_outputStream = &outputStream;
  }
  std::ostream& getOutputStream()
  {
    return *m_outputStream;
  }
};

typedef Static<OutputStreamHolder> GlobalOutputStream;

/// \brief Returns the global output stream. Used to display messages to the user.
inline std::ostream& globalOutputStream()
{
  return GlobalOutputStream::instance().getOutputStream();
}

typedef Static<OutputStreamHolder> GlobalErrorStream;

/// \brief Returns the global error stream. Used to display error messages to the user.
inline std::ostream& globalErrorStream()
{
  return GlobalErrorStream::instance().getOutputStream();
}

#endif
