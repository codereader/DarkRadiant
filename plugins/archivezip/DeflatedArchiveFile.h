#pragma once

#include "iarchive.h"
#include "stream/FileInputStream.h"
#include "DeflatedInputStream.h"

namespace archive
{

class DeflatedArchiveFile :
	public ArchiveFile
{
private:
	std::string _name;
	stream::FileInputStream _istream;
	stream::SubFileInputStream _substream;	// provides a subset of _istream
	DeflatedInputStream _zipstream; // inflates data from _subStream
	stream::FileInputStream::size_type _size;

public:
	typedef stream::FileInputStream::size_type size_type;
	typedef stream::FileInputStream::position_type position_type;

	DeflatedArchiveFile(const std::string& name,
						const std::string& archiveName, // path to the ZIP file
						position_type position,
						size_type stream_size,
						size_type file_size) :
		_name(name),
		_istream(archiveName),
		_substream(_istream, position, stream_size),
		_zipstream(_substream), 
		_size(file_size)
	{}

	size_type size() const override
	{
		return _size;
	}

	const std::string& getName() const override
	{
		return _name;
	}

	InputStream& getInputStream() override
	{
		return _zipstream;
	}
};

}
