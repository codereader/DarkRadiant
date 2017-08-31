#pragma once

#include "iarchive.h"

namespace archive
{

/// \brief An ArchiveFile which is stored uncompressed as part of a larger archive file.
class StoredArchiveFile :
	public ArchiveFile
{
private:
	std::string _name;
	stream::FileInputStream _filestream;
	stream::SubFileInputStream _substream;	// provides a subset of _filestream
	stream::FileInputStream::size_type _size;

public:
	typedef stream::FileInputStream::size_type size_type;
	typedef stream::FileInputStream::position_type position_type;

	StoredArchiveFile(const std::string& name,
					  const std::string& archiveName, // full path to the archive file
					  position_type position,
					  size_type stream_size,
					  size_type file_size) : 
		_name(name),
		_filestream(archiveName),
		_substream(_filestream, position, stream_size),
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
		return _substream;
	}
};

}
