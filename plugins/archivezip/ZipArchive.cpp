#include "ZipArchive.h"

#include "itextstream.h"
#include "iarchive.h"
#include "archivelib.h"

#include "pkzip.h"
#include "zlibstream.h"
#include "os/fs.h"
#include "os/path.h"

#include "DeflatedArchiveFile.h"
#include "DeflatedArchiveTextFile.h"

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

	if (i != _filesystem.end() && !i->second.is_directory())
	{
		const std::shared_ptr<ZipRecord>& file = i->second.file();

		FileInputStream::size_type position = 0;

		{
			// Guard against concurrent access
			std::lock_guard<std::mutex> lock(_streamLock);

			_istream.seek(file->position);
			zip_file_header file_header;
			istream_read_zip_file_header(_istream, file_header);
			position = _istream.tell();

			if (file_header.z_magic != zip_file_header_magic)
			{
				rError() << "error reading zip file " << _fullPath << std::endl;
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

	if (i != _filesystem.end() && !i->second.is_directory())
	{
		const std::shared_ptr<ZipRecord>& file = i->second.file();

		{
			// Guard against concurrent access
			std::lock_guard<std::mutex> lock(_streamLock);

			_istream.seek(file->position);
			zip_file_header file_header;
			istream_read_zip_file_header(_istream, file_header);

			if (file_header.z_magic != zip_file_header_magic)
			{
				rError() << "error reading zip file " << _fullPath << std::endl;
				return ArchiveTextFilePtr();
			}
		}

		switch (file->mode)
		{
		case ZipRecord::eStored:
			return std::make_shared<StoredArchiveTextFile>(name,
				_fullPath,
				_containingFolder,
				_istream.tell(),
				file->stream_size);

		case ZipRecord::eDeflated:
			return std::make_shared<DeflatedArchiveTextFile>(name,
				_fullPath,
				_containingFolder,
				_istream.tell(),
				file->stream_size);
		}
	}

	return ArchiveTextFilePtr();
}

bool ZipArchive::containsFile(const std::string& name) {
	ZipFileSystem::iterator i = _filesystem.find(name);
	return i != _filesystem.end() && !i->second.is_directory();
}

void ZipArchive::forEachFile(VisitorFunc visitor, const std::string& root) {
	_filesystem.traverse(visitor, root);
}

void ZipArchive::readZipRecord()
{
	zip_magic magic;
	istream_read_zip_magic(_istream, magic);

	if (!(magic == zip_root_dirent_magic))
	{
		throw ZipFailureException("Invalid Zip directory entry magic");
	}
	zip_version version_encoder;
	istream_read_zip_version(_istream, version_encoder);
	zip_version version_extract;
	istream_read_zip_version(_istream, version_extract);

	//unsigned short flags =
	stream::readLittleEndian<int16_t>(_istream);
	
	uint16_t compression_mode = stream::readLittleEndian<uint16_t>(_istream);

	if (compression_mode != Z_DEFLATED && compression_mode != 0)
	{
		throw ZipFailureException("Unsupported compression mode");
	}

	zip_dostime dostime;
	istream_read_zip_dostime(_istream, dostime);

	//unsigned int crc32 =
	istream_read_int32_le(_istream);

	unsigned int compressed_size = istream_read_uint32_le(_istream);
	unsigned int uncompressed_size = istream_read_uint32_le(_istream);
	unsigned int namelength = istream_read_uint16_le(_istream);
	unsigned short extras = istream_read_uint16_le(_istream);
	unsigned short comment = istream_read_uint16_le(_istream);

	//unsigned short diskstart =
	istream_read_int16_le(_istream);
	//unsigned short filetype =
	istream_read_int16_le(_istream);
	//unsigned int filemode =
	istream_read_int32_le(_istream);

	unsigned int position = istream_read_int32_le(_istream);

	// greebo: Read the filename directly into a newly constructed std::string.

	// I'm not entirely happy about this brute-force casting, but I wanted to
	// avoid reading the filename into a temporary char[] array
	// only to let its contents end up being copied by the std::string anyway.
	// Alternative: use a static boost::shared_array here, resized to fit?

	std::string path(namelength, '\0');

	_istream.read(
		reinterpret_cast<FileInputStream::byte_type*>(const_cast<char*>(path.data())),
		namelength);

	_istream.seek(extras + comment, FileInputStream::cur);

	if (os::isDirectory(path))
	{
		_filesystem[path].file().reset();
	}
	else
	{
		ZipFileSystem::entry_type& file = _filesystem[path];

		if (!file.is_directory())
		{
			rWarning() << "Zip archive " << _fullPath << " contains duplicated file: " << path << std::endl;
		}
		else
		{
			file.file().reset(new ZipRecord(position,
				compressed_size,
				uncompressed_size,
				(compression_mode == Z_DEFLATED) ? ZipRecord::eDeflated : ZipRecord::eStored));
		}
	}
}

void ZipArchive::loadZipFile()
{
	SeekableStream::position_type pos = pkzip_find_disk_trailer(_istream);

	if (pos == 0)
	{
		throw ZipFailureException("Unable to locate Zip disk trailer");
	}

	_istream.seek(pos);

	zip_disk_trailer disk_trailer;
	istream_read_zip_disk_trailer(_istream, disk_trailer);

	if (disk_trailer.z_magic != zip_disk_trailer_magic)
	{
		throw ZipFailureException("Invalid Zip Magic, maybe this is not a zip file?");
	}

	_istream.seek(disk_trailer.z_rootseek);

	for (unsigned short i = 0; i < disk_trailer.z_entries; ++i)
	{
		readZipRecord();
	}
}

}
