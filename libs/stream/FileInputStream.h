#pragma once

#include "idatastream.h"
#include <algorithm>
#include <cstdio>

namespace stream
{

namespace detail
{

inline int whence_for_seekdir(SeekableStream::seekdir direction)
{
	switch (direction)
	{
	case SeekableStream::cur:
		return SEEK_CUR;
	case SeekableStream::end:
		return SEEK_END;
	default:
		break;
	}
	return SEEK_SET;
}

}

/// \brief A wrapper around a file input stream opened for reading in binary mode. Similar to std::ifstream.
///
/// - Maintains a valid file handle associated with a name passed to the constructor.
/// - Implements SeekableInputStream.
class FileInputStream : 
	public SeekableInputStream
{
private:
	std::FILE* _file;
public:
	FileInputStream(const std::string& name) :
		_file(!name.empty() ? fopen(name.c_str(), "rb") : nullptr)
	{}

	~FileInputStream()
	{
		if (!failed())
		{
			fclose(_file);
		}
	}

	bool failed() const
	{
		return _file == nullptr;
	}

	size_type read(byte_type* buffer, size_type length) override
	{
		return fread(buffer, 1, length, _file);
	}

	size_type seek(size_type position) override
	{
		return fseek(_file, static_cast<long>(position), SEEK_SET);
	}

	size_type seek(offset_type offset, seekdir direction) override
	{
		return fseek(_file, offset, detail::whence_for_seekdir(direction));
	}

	size_type tell() const override
	{
		return ftell(_file);
	}

	std::FILE* file()
	{
		return _file;
	}
};

/// \brief A wrapper around a FileInputStream limiting access.
///
/// - Maintains an input stream.
/// - Provides input starting at an offset in the file for a limited range.
class SubFileInputStream : 
	public InputStream
{
private:
	FileInputStream& _istream;
	size_type _remaining;

public:
	typedef FileInputStream::position_type position_type;

	SubFileInputStream(FileInputStream& istream, position_type offset, size_type size) : 
		_istream(istream), 
		_remaining(size)
	{
		_istream.seek(offset);
	}

	size_type read(byte_type* buffer, size_type length) override
	{
		size_type result = _istream.read(buffer, std::min(length, _remaining));
		_remaining -= result;
		return result;
	}
};

}
