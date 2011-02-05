#pragma once

#include "itextstream.h"
#include <algorithm>

class BufferInputStream : 
	public TextInputStream
{
private:
	const char* _begin;
	const char* _read;
	const char* _end;

public:
	BufferInputStream(const char* buffer, std::size_t length) :
		_begin(buffer),
		_read(buffer), 
		_end(buffer + length)
	{}

	std::size_t read(char* buffer, std::size_t length)
	{
		std::size_t count = std::min(std::size_t(_end - _read), length);
		
		const char* end = _read + count;

		while (_read != end)
		{
			*buffer++ = *_read++;
		}

		return count;
	}

	// greebo: Override default std::streambuf::seekoff() method to provide buffer positioning capabilities
	virtual std::streampos seekoff(std::streamoff off,
								   std::ios_base::seekdir way,
								   std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
	{
		if (way == std::ios_base::beg)
		{
			const char* newPos = _begin + off;

			// Force streambuf underflow
			setg(_buffer, _buffer, _buffer);

			if (newPos > _end || newPos < _begin)
			{
				return std::streampos(-1); // error
			}

			_read = newPos;
		}
		else if (way == std::ios_base::cur)
		{
			const char* newPos = _read + off;

			if (newPos > _end || newPos < _begin)
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

			_read = newPos;
		}
		else if (way == std::ios_base::end)
		{
			const char* newPos = _end + off;

			// Force streambuf underflow
			setg(_buffer, _buffer, _buffer);

			if (newPos > _end || newPos < _begin)
			{
				return std::streampos(-1); // error
			}

			_read = newPos;
		}

		return std::streampos(_read - _begin);
	}
};
