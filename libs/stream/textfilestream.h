#pragma once

#include "itextstream.h"
#include <stdio.h>

// A wrapper around a file input stream opened for reading in text mode. Similar to std::ifstream.
class TextFileInputStream : 
	public TextInputStream
{
private:
	FILE* _file;

public:
	TextFileInputStream(const std::string& name) :
		_file(!name.empty() ? fopen(name.c_str(), "rt") : NULL)
	{}

	virtual ~TextFileInputStream()
	{
		if (!failed())
		{
			fclose(_file);
		}
	}

	bool failed() const
	{
		return _file == 0;
	}

	std::size_t read(char* buffer, std::size_t length)
	{
		return fread(buffer, 1, length, _file);
	}

	// greebo: Override default std::streambuf::seekoff() method to provide buffer positioning capabilities
	virtual std::streampos seekoff(std::streamoff off,
								   std::ios_base::seekdir way,
								   std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
	{
		if (way == std::ios_base::beg)
		{
			// Invalidate the buffer of our base class to force an underflow
			setg(_buffer, _buffer, _buffer);

			if (fseek(_file, static_cast<long>(off), SEEK_SET))
			{
				return std::streampos(-1); // error
			}
		}
		else if (way == std::ios_base::cur)
		{
			if (fseek(_file, static_cast<long>(off), SEEK_CUR))
			{
				return std::streampos(-1); // error
			}
			else
			{
				// success, check if we need to invalidate our controlled input sequence
				if (gptr() + off > egptr() || gptr() + off < eback())
				{
					setg(_buffer, _buffer, _buffer);
				}
			}
		}
		else if (way == std::ios_base::end)
		{
			// Invalidate the buffer of our base class to force an underflow
			setg(_buffer, _buffer, _buffer);

			if (fseek(_file, static_cast<long>(off), SEEK_END))
			{
				return std::streampos(-1); // error
			}
		}
		
		return std::streampos(ftell(_file));
	}
};
