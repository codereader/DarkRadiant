#include "ZipArchive.h"

#include <stdexcept>
#include "itextstream.h"
#include "iarchive.h"
#include "gamelib.h"
#include <zlib.h>

#include "os/fs.h"
#include "os/path.h"

#include "ZipStreamUtils.h"
#include "DeflatedArchiveFile.h"
#include "DeflatedArchiveTextFile.h"
#include "StoredArchiveFile.h"
#include "StoredArchiveTextFile.h"

namespace archive
{

// Thrown by the zip reader methods below
class ZipFailureException :
	public std::runtime_error
{
public:
	ZipFailureException(const char* msg) :
		std::runtime_error(msg)
	{}
};


ZipArchive::ZipArchive(const std::string& fullPath) :
	_fullPath(fullPath),
	_containingFolder(os::standardPathWithSlash(fs::path(_fullPath).remove_filename())),
	_istream(_fullPath)
{
	if (_istream.failed())
	{
		rError() << "Cannot open Zip file stream: " << _fullPath << std::endl;
		return;
	}

	try
	{
		// Try loading the zip file, this will throw exceptoions on any problem
		loadZipFile();
	}
	catch (ZipFailureException& ex)
	{
		rError() << "Cannot read Zip file " << _fullPath << ": " << ex.what() << std::endl;
	}
}

ZipArchive::~ZipArchive()
{
	_filesystem.clear();
}

ArchiveFilePtr ZipArchive::openFile(const std::string& name)
{
	ZipFileSystem::iterator i = _filesystem.find(name);

	if (i != _filesystem.end() && !i->second.isDirectory())
	{
		const std::shared_ptr<ZipRecord>& file = i->second.getRecord();

		stream::FileInputStream::size_type position = 0;

		{
			// Guard against concurrent access
			std::lock_guard<std::mutex> lock(_streamLock);

			_istream.seek(file->position);

			ZipFileHeader header;
			stream::readZipFileHeader(_istream, header);

			position = _istream.tell();

			if (header.magic != ZIP_MAGIC_FILE_HEADER)
			{
				rError() << "Error reading zip file " << _fullPath << std::endl;
				return ArchiveFilePtr();
			}
		}

		switch (file->mode)
		{
		case ZipRecord::eStored:
			return std::make_shared<StoredArchiveFile>(name, _fullPath, position, file->stream_size, file->file_size);
		case ZipRecord::eDeflated:
			return std::make_shared<DeflatedArchiveFile>(name, _fullPath, position, file->stream_size, file->file_size);
		}
	}

	return ArchiveFilePtr();
}

ArchiveTextFilePtr ZipArchive::openTextFile(const std::string& name)
{
	ZipFileSystem::iterator i = _filesystem.find(name);

	if (i != _filesystem.end() && !i->second.isDirectory())
	{
		const std::shared_ptr<ZipRecord>& file = i->second.getRecord();

		// Guard against concurrent access
		std::lock_guard<std::mutex> lock(_streamLock);

		_istream.seek(file->position);

		ZipFileHeader header;
		stream::readZipFileHeader(_istream, header);

		if (header.magic != ZIP_MAGIC_FILE_HEADER)
		{
			rError() << "Error reading zip file " << _fullPath << std::endl;
			return ArchiveTextFilePtr();
		}

		switch (file->mode)
		{
		case ZipRecord::eStored:
			return std::make_shared<StoredArchiveTextFile>(
                name, _fullPath, _containingFolder, _istream.tell(), file->stream_size
            );

		case ZipRecord::eDeflated:
			return std::make_shared<DeflatedArchiveTextFile>(
                name, _fullPath, _containingFolder, _istream.tell(), file->stream_size
            );
		}
	}

	return ArchiveTextFilePtr();
}

bool ZipArchive::containsFile(const std::string& name)
{
	ZipFileSystem::iterator i = _filesystem.find(name);
	return i != _filesystem.end() && !i->second.isDirectory();
}

void ZipArchive::traverse(Visitor& visitor, const std::string& root)
{
	_filesystem.traverse(visitor, root);
}

void ZipArchive::readZipRecord()
{
	ZipMagic magic;
	stream::readZipMagic(_istream, magic);

	if (magic != ZIP_MAGIC_ROOT_DIR_ENTRY)
	{
		throw ZipFailureException("Invalid Zip directory entry magic");
	}

	ZipVersion version_encoder;
	stream::readZipVersion(_istream, version_encoder);
	ZipVersion version_extract;
	stream::readZipVersion(_istream, version_extract);

	//unsigned short flags =
	stream::readLittleEndian<int16_t>(_istream);
	
	uint16_t compression_mode = stream::readLittleEndian<uint16_t>(_istream);

	if (compression_mode != Z_DEFLATED && compression_mode != 0)
	{
		throw ZipFailureException("Unsupported compression mode");
	}

	ZipDosTime dostime;
	stream::readZipDosTime(_istream, dostime);

	//unsigned int crc32 =
	stream::readLittleEndian<uint32_t>(_istream);
	
	uint32_t compressed_size = stream::readLittleEndian<uint32_t>(_istream);
	uint32_t uncompressed_size = stream::readLittleEndian<uint32_t>(_istream);
	uint16_t namelength = stream::readLittleEndian<uint16_t>(_istream);
	uint16_t extras = stream::readLittleEndian<uint16_t>(_istream);
	uint16_t comment = stream::readLittleEndian<uint16_t>(_istream);

	//unsigned short diskstart =
	stream::readLittleEndian<uint16_t>(_istream);
	//unsigned short filetype =
	stream::readLittleEndian<uint16_t>(_istream);
	//unsigned int filemode =
	stream::readLittleEndian<uint32_t>(_istream);

	uint32_t position = stream::readLittleEndian<uint32_t>(_istream);

	// greebo: Read the filename directly into a newly constructed std::string.

	// I'm not entirely happy about this brute-force casting, but I wanted to
	// avoid reading the filename into a temporary char[] array
	// only to let its contents end up being copied by the std::string anyway.
	// Alternative: use a static std::shared_array here, resized to fit?

	std::string path(namelength, '\0');

	_istream.read(
		reinterpret_cast<stream::FileInputStream::byte_type*>(const_cast<char*>(path.data())),
		namelength);

	_istream.seek(extras + comment, stream::FileInputStream::cur);

	if (os::isDirectory(path))
	{
		_filesystem[path].getRecord().reset();
	}
	else
	{
		ZipFileSystem::entry_type& entry = _filesystem[path];

		if (!entry.isDirectory())
		{
			rWarning() << "Zip archive " << _fullPath << " contains duplicated file: " << path << std::endl;
		}
		else
		{
			entry.getRecord().reset(new ZipRecord(position,
				compressed_size,
				uncompressed_size,
				(compression_mode == Z_DEFLATED) ? ZipRecord::eDeflated : ZipRecord::eStored));
		}
	}
}

void ZipArchive::loadZipFile()
{
	SeekableStream::position_type pos = findZipDiskTrailerPosition(_istream);

	if (pos == 0)
	{
		throw ZipFailureException("Unable to locate Zip disk trailer");
	}

	_istream.seek(pos);

	ZipDiskTrailer trailer;
	stream::readZipDiskTrailer(_istream, trailer);

	if (trailer.magic != ZIP_MAGIC_DISK_TRAILER)
	{
		throw ZipFailureException("Invalid Zip Magic, maybe this is not a zip file?");
	}

	_istream.seek(trailer.rootseek);

	for (unsigned short i = 0; i < trailer.entries; ++i)
	{
		readZipRecord();
	}
}

}
