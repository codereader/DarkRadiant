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

#if !defined(INCLUDED_STREAM_TEXTFILESTREAM_H)
#define INCLUDED_STREAM_TEXTFILESTREAM_H

#include "itextstream.h"
#include <stdio.h>

/// \brief A wrapper around a file input stream opened for reading in text mode. Similar to std::ifstream.
class TextFileInputStream : public TextInputStream
{
  FILE* m_file;
public:
	TextFileInputStream(const std::string& name) :
		m_file(!name.empty() ? fopen(name.c_str(), "rt") : NULL)
	{}
  
  ~TextFileInputStream()
  {
    if(!failed())
      fclose(m_file);
  }

  bool failed() const
  {
    return m_file == 0;
  }

  std::size_t read(char* buffer, std::size_t length)
  {
    return fread(buffer, 1, length, m_file);
  }

	// greebo: Override default std::streambuf::seekoff() method to provide buffer positioning capabilities
	virtual std::streampos seekoff(std::streamoff off, 
								   std::ios_base::seekdir way, 
								   std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
	{
		if (way == std::ios_base::beg)
		{
			if (fseek(m_file, static_cast<long>(off), SEEK_SET))
			{
				return std::streampos(-1); // error
			}
		}
		else if (way == std::ios_base::cur)
		{
			if (fseek(m_file, static_cast<long>(off), SEEK_CUR))
			{
				return std::streampos(-1); // error
			}
		}
		else if (way == std::ios_base::end)
		{
			if (fseek(m_file, static_cast<long>(off), SEEK_END))
			{
				return std::streampos(-1); // error
			}
		}

		return std::streampos(ftell(m_file));
	}
};

#endif
