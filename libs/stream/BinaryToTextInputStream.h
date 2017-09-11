#pragma once

#include "itextstream.h"

namespace stream
{

/// \brief A single-byte-reader wrapper around an InputStream.
/// Optimised for reading one byte at a time.
/// Uses a buffer to reduce the number of times the wrapped stream must be read.
template<typename InputStreamType>
class SingleByteInputStream
{
private:
	typedef typename InputStreamType::byte_type byte_type;

	static const std::size_t SIZE = 1024;

	InputStreamType& _inputStream;
	byte_type _buffer[SIZE];
	byte_type* _cur;
	byte_type* _end;

public:
	SingleByteInputStream(InputStreamType& inputStream) : 
		_inputStream(inputStream), 
		_cur(_buffer + SIZE), 
		_end(_cur)
	{}

	bool readByte(byte_type& b)
	{
		if (_cur == _end)
		{
			if (_end != _buffer + SIZE)
			{
				return false;
			}

			_end = _buffer + _inputStream.read(_buffer, SIZE);
			_cur = _buffer;

			if (_end == _buffer)
			{
				return false;
			}
		}

		b = *_cur++;

		return true;
	}
};

/// \brief A binary-to-text wrapper around an InputStream.
/// Converts CRLF or LFCR line-endings to LF line-endings.
template<typename BinaryInputStreamType>
class BinaryToTextInputStream : 
	public TextInputStream
{
private:
	SingleByteInputStream<BinaryInputStreamType> _inputStream;

public:
	BinaryToTextInputStream(BinaryInputStreamType& inputStream) : 
		_inputStream(inputStream)
	{}

	std::size_t read(char* buffer, std::size_t length) override
	{
		char* p = buffer;
		for (;;)
		{
			if (length != 0 && _inputStream.readByte(*reinterpret_cast<typename BinaryInputStreamType::byte_type*>(p)))
			{
				if (*p != '\r')
				{
					++p;
					--length;
				}
			}
			else
			{
				return p - buffer;
			}
		}
	}
};

}

