#include "ZipArchive.h"

#include "iarchive.h"
#include "archivelib.h"

#include "pkzip.h"
#include "zlibstream.h"

#include "DeflatedArchiveFile.h"
#include "DeflatedArchiveTextFile.h"

ZipArchive::ZipArchive(const std::string& name) :
	m_name(name), 
	m_istream(name)
{
	if (!m_istream.failed()) {
		if (!read_pkzip()) {
			globalErrorStream() << "ERROR: invalid zip-file " << name.c_str() << '\n';
		}
	}
}

ZipArchive::~ZipArchive() {
	for (ZipFileSystem::iterator i = m_filesystem.begin(); 
		 i != m_filesystem.end(); ++i)
	{
		delete i->second.file();
	}
}

bool ZipArchive::failed() {
	return m_istream.failed();
}

ArchiveFilePtr ZipArchive::openFile(const std::string& name) {
	ZipFileSystem::iterator i = m_filesystem.find(name);
	if (i != m_filesystem.end() && !i->second.is_directory()) {
		ZipRecord* file = i->second.file();

		m_istream.seek(file->m_position);
		zip_file_header file_header;
		istream_read_zip_file_header(m_istream, file_header);
		
		if (file_header.z_magic != zip_file_header_magic) {
			globalErrorStream() << "error reading zip file " << m_name.c_str();
			return ArchiveFilePtr();
		}

		switch (file->m_mode) {
			case ZipRecord::eStored:
				return ArchiveFilePtr(new StoredArchiveFile(name, m_name, m_istream.tell(), file->m_stream_size, file->m_file_size));
			case ZipRecord::eDeflated:
				return ArchiveFilePtr(new DeflatedArchiveFile(name, m_name, m_istream.tell(), file->m_stream_size, file->m_file_size));
		}
	}
	return ArchiveFilePtr();
}

ArchiveTextFilePtr ZipArchive::openTextFile(const std::string& name) {
	ZipFileSystem::iterator i = m_filesystem.find(name);
	if (i != m_filesystem.end() && !i->second.is_directory()) {
		ZipRecord* file = i->second.file();

		m_istream.seek(file->m_position);
		zip_file_header file_header;
		istream_read_zip_file_header(m_istream, file_header);
		
		if (file_header.z_magic != zip_file_header_magic) {
			globalErrorStream() << "error reading zip file " << m_name.c_str();
			return ArchiveTextFilePtr();
		}

		switch (file->m_mode) {
			case ZipRecord::eStored:
				return ArchiveTextFilePtr(new StoredArchiveTextFile(name,
					m_name,
					m_name,
					m_istream.tell(),
					file->m_stream_size));
			case ZipRecord::eDeflated:
				return ArchiveTextFilePtr(new DeflatedArchiveTextFile(name,
					m_name,
					m_name,
					m_istream.tell(),
					file->m_stream_size));
		}
	}
	return ArchiveTextFilePtr();
}

bool ZipArchive::containsFile(const std::string& name) {
	ZipFileSystem::iterator i = m_filesystem.find(name);
	return i != m_filesystem.end() && !i->second.is_directory();
}

void ZipArchive::forEachFile(VisitorFunc visitor, const std::string& root) {
	m_filesystem.traverse(visitor, root);
}

bool ZipArchive::read_record() {
	zip_magic magic;
	istream_read_zip_magic(m_istream, magic);
	
	if (!(magic == zip_root_dirent_magic)) {
		return false;
	}
	zip_version version_encoder;
	istream_read_zip_version(m_istream, version_encoder);
	zip_version version_extract;
	istream_read_zip_version(m_istream, version_extract);
	//unsigned short flags = 
	istream_read_int16_le(m_istream);
	unsigned short compression_mode = istream_read_int16_le(m_istream);

	if (compression_mode != Z_DEFLATED && compression_mode != 0) {
		return false;
	}

	zip_dostime dostime;
	istream_read_zip_dostime(m_istream, dostime);

	//unsigned int crc32 = 
	istream_read_int32_le(m_istream);

	unsigned int compressed_size = istream_read_uint32_le(m_istream);
	unsigned int uncompressed_size = istream_read_uint32_le(m_istream);
	unsigned int namelength = istream_read_uint16_le(m_istream);
	unsigned short extras = istream_read_uint16_le(m_istream);
	unsigned short comment = istream_read_uint16_le(m_istream);

	//unsigned short diskstart =
	istream_read_int16_le(m_istream);
	//unsigned short filetype = 
	istream_read_int16_le(m_istream);
	//unsigned int filemode =
	istream_read_int32_le(m_istream);

	unsigned int position = istream_read_int32_le(m_istream);

	// greebo: Read the filename directly into a newly constructed std::string.

	// I'm not entirely happy about this brute-force casting, but I wanted to 
	// avoid reading the filename into a temporary char[] array 
	// only to let its contents end up being copied by the std::string anyway.
	// Alternative: use a static boost::shared_array here, resized to fit?

	std::string path(namelength, '\0');

	m_istream.read(
		reinterpret_cast<FileInputStream::byte_type*>(const_cast<char*>(path.data())),
		namelength);

	m_istream.seek(extras + comment, FileInputStream::cur);

	if (path_is_directory(path.c_str())) {
		m_filesystem[path] = 0;
	} 
	else {
		ZipFileSystem::entry_type& file = m_filesystem[path];
		if (!file.is_directory()) {
			globalOutputStream() << "Warning: zip archive "
				<< m_name << " contains duplicated file: "
				<< path << std::endl;
		} 
		else {
			file = new ZipRecord(position, 
								 compressed_size,
								 uncompressed_size, 
								 (compression_mode == Z_DEFLATED) ? ZipRecord::eDeflated : ZipRecord::eStored);
		}
	}

	return true;
}

bool ZipArchive::read_pkzip() {
	SeekableStream::position_type pos = pkzip_find_disk_trailer(m_istream);
	if (pos != 0) {
		zip_disk_trailer disk_trailer;
		
		m_istream.seek(pos);
		istream_read_zip_disk_trailer(m_istream, disk_trailer);
		
		if (!(disk_trailer.z_magic == zip_disk_trailer_magic)) {
			return false;
		}

		m_istream.seek(disk_trailer.z_rootseek);
		
		for (unsigned int i = 0; i < disk_trailer.z_entries; ++i) {
			if (!read_record()) {
				return false;
			}
		}
		
		return true;
	}
	return false;
}
