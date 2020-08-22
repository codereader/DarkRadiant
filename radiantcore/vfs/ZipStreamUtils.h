#pragma once

#include "stream/utils.h"
#include "idatastream.h"
#include <algorithm>

/**
 * greebo: Various data structures as defined by the PKWARE
 * ZIP File Format Specification, plus a few stream helper functions.
 */
namespace archive
{

class ZipMagic
{
public:
	ZipMagic()
	{}

	ZipMagic(char c0, char c1, char c2, char c3)
	{
		value[0] = c0;
		value[1] = c1;
		value[2] = c2;
		value[3] = c3;
	}

	bool operator==(const ZipMagic& other) const
	{
		return value[0] == other.value[0]
			&& value[1] == other.value[1]
			&& value[2] == other.value[2]
			&& value[3] == other.value[3];
	}

	bool operator!=(const ZipMagic& other) const
	{
		return !(*this == other);
	}

	char value[4];
};

const ZipMagic ZIP_MAGIC_FILE_HEADER('P', 'K', 0x03, 0x04);

struct ZipVersion
{
	char version;
	char ostype;
};

struct ZipDosTime
{
	uint16_t time;
	uint16_t date;
};

/* A. Local file header */
struct ZipFileHeader
{
	ZipMagic magic;				/* local file header signature (0x04034b50) */
	ZipVersion extract;			/* version needed to extract */
	uint16_t flags;				/* general purpose bit flag */
	uint16_t compressionMethod;	/* compression method */
	ZipDosTime dosTime;			/* last mod file time (dos format) */
	uint32_t crc32;				/* crc-32 */
	uint32_t compressedSize;	/* compressed size */
	uint32_t uncompressedSize;	/* uncompressed size */
	uint16_t nameLength;		/* filename length (null if stdin) */
	uint16_t extras;			/* extra field length */
								/* followed by filename (of variable size) */
								/* followed by extra field (of variable size) */
};

/* B. data descriptor
* the data descriptor exists only if bit 3 of z_flags is set. It is byte aligned
* and immediately follows the last byte of compressed data. It is only used if
* the output media of the compressor was not seekable, eg. standard output.
*/
const ZipMagic ZIP_MAGIC_FILE_TRAILER('P', 'K', 0x07, 0x08);

struct ZipFileTrailer
{
	ZipMagic magic;
	uint32_t crc32;
	uint32_t compressedSize;
	uint32_t uncompressedSize;
};

/* C. central directory structure:
[file header] . . . end of central dir record
*/

/* directory file header
* - a single entry including filename, extras and comment may not exceed 64k.
*/
const ZipMagic ZIP_MAGIC_ROOT_DIR_ENTRY('P', 'K', 0x01, 0x02);

struct ZipRootDirEntry
{
	ZipMagic magic;
	ZipVersion encoder;			/* version made by */
	ZipVersion extract;			/* version need to extract */
	uint16_t flags;				/* general purpose bit flag */
	uint16_t compressionMethod;	/* compression method */
	ZipDosTime dostime;			/* last mod file time&date (dos format) */
	uint32_t crc32;				/* crc-32 */
	uint32_t compressedSize;	/* compressed size */
	uint32_t uncompressedSize;	/* uncompressed size */
	uint16_t nameLength;		/* filename length (null if stdin) */
	uint16_t extras;			/* extra field length */
	uint16_t comment;			/* file comment length */
	uint16_t diskstart;			/* disk number of start (if spanning zip over multiple disks) */
	uint16_t filetype;			/* internal file attributes, bit0 = ascii */
	uint32_t filemode;			/* extrnal file attributes, eg. msdos attrib byte */
	uint32_t offset;			/* relative offset of local file header, seekval if singledisk */
								/* followed by filename (of variable size) */
								/* followed by extra field (of variable size) */
								/* followed by file comment (of variable size) */
};

/* end of central dir record */
const ZipMagic ZIP_MAGIC_DISK_TRAILER('P', 'K', 0x05, 0x06);

struct ZipDiskTrailer
{
	ZipMagic magic;
	uint16_t disk;			/* number of this disk */
	uint16_t finaldisk;		/* number of the disk with the start of the central dir */
	uint16_t entries;		/* total number of entries in the central dir on this disk */
	uint16_t finalentries;	/* total number of entries in the central dir */
	uint32_t rootsize;		/* size of the central directory */
	uint32_t rootseek;		/* offset of start of central directory with respect to
							 * the starting disk number */
	uint16_t comment;		/* zipfile comment length */
							/* followed by zipfile comment (of variable size) */
};

const std::size_t ZIP_DISK_TRAILER_LENGTH = 22;

}

// Convenience functions, reading Zip structures from an InputStream
namespace stream
{

inline void readZipMagic(InputStream& stream, archive::ZipMagic& magic)
{
	stream.read(reinterpret_cast<InputStream::byte_type*>(magic.value), 4);
}

inline void readZipVersion(InputStream& stream, archive::ZipVersion& version)
{
	version.version = stream::readByte(stream);
	version.ostype = stream::readByte(stream);
}

inline void readZipDosTime(InputStream& stream, archive::ZipDosTime& dostime)
{
	dostime.time = stream::readLittleEndian<uint16_t>(stream);
	dostime.date = stream::readLittleEndian<uint16_t>(stream);
}

inline void readZipFileHeader(SeekableInputStream& stream, archive::ZipFileHeader& header)
{
	stream::readZipMagic(stream, header.magic);
	stream::readZipVersion(stream, header.extract);
	header.flags = stream::readLittleEndian<uint16_t>(stream);
	header.compressionMethod = stream::readLittleEndian<uint16_t>(stream);
	stream::readZipDosTime(stream, header.dosTime);
	header.crc32 = stream::readLittleEndian<uint32_t>(stream);
	header.compressedSize = stream::readLittleEndian<uint32_t>(stream);
	header.uncompressedSize = stream::readLittleEndian<uint32_t>(stream);
	header.nameLength = stream::readLittleEndian<uint16_t>(stream);
	header.extras = stream::readLittleEndian<uint16_t>(stream);

	stream.seek(header.nameLength + header.extras, SeekableInputStream::cur);
};

inline void readZipFileTrailer(InputStream& stream, archive::ZipFileTrailer& trailer)
{
	stream::readZipMagic(stream, trailer.magic);
	trailer.crc32 = stream::readLittleEndian<uint32_t>(stream);
	trailer.compressedSize = stream::readLittleEndian<uint32_t>(stream);
	trailer.uncompressedSize = stream::readLittleEndian<uint32_t>(stream);
};

inline void readZipRootDirEntry(SeekableInputStream& stream, archive::ZipRootDirEntry& entry)
{
	stream::readZipMagic(stream, entry.magic);
	stream::readZipVersion(stream, entry.encoder);
	stream::readZipVersion(stream, entry.extract);
	entry.flags = stream::readLittleEndian<uint16_t>(stream);
	entry.compressionMethod = stream::readLittleEndian<uint16_t>(stream);
	stream::readZipDosTime(stream, entry.dostime);
	entry.crc32 = stream::readLittleEndian<uint32_t>(stream);
	entry.compressedSize = stream::readLittleEndian<uint32_t>(stream);
	entry.uncompressedSize = stream::readLittleEndian<uint32_t>(stream);
	entry.nameLength = stream::readLittleEndian<uint16_t>(stream);
	entry.extras = stream::readLittleEndian<uint16_t>(stream);
	entry.comment = stream::readLittleEndian<uint16_t>(stream);
	entry.diskstart = stream::readLittleEndian<uint16_t>(stream);
	entry.filetype = stream::readLittleEndian<uint16_t>(stream);
	entry.filemode = stream::readLittleEndian<uint32_t>(stream);
	entry.offset = stream::readLittleEndian<uint32_t>(stream);

	stream.seek(entry.nameLength + entry.extras + entry.comment, SeekableInputStream::cur);
}

inline void readZipDiskTrailer(SeekableInputStream& stream, archive::ZipDiskTrailer& trailer)
{
	stream::readZipMagic(stream, trailer.magic);
	trailer.disk = stream::readLittleEndian<uint16_t>(stream);
	trailer.finaldisk = stream::readLittleEndian<uint16_t>(stream);
	trailer.entries = stream::readLittleEndian<uint16_t>(stream);
	trailer.finalentries = stream::readLittleEndian<uint16_t>(stream);
	trailer.rootsize = stream::readLittleEndian<uint32_t>(stream);
	trailer.rootseek = stream::readLittleEndian<uint32_t>(stream);
	trailer.comment = stream::readLittleEndian<uint16_t>(stream);

	stream.seek(trailer.comment, SeekableInputStream::cur);
}

} // namespace

namespace archive
{

// Function trying to locate the trailer position in the given seekable stream.
// Will return the position of the trailer structure, or 0 if it was unable to find it.
inline SeekableStream::position_type findZipDiskTrailerPosition(SeekableInputStream& stream)
{
	// Seek to the end of the file and check if the last 22 bytes match the Zip Disk Trailer
	stream.seek(0, SeekableInputStream::end);

	SeekableStream::position_type startPosition = stream.tell();

	if (startPosition < ZIP_DISK_TRAILER_LENGTH)
	{
		return 0; // file is too small
	}

	startPosition -= ZIP_DISK_TRAILER_LENGTH;

	stream.seek(startPosition);

	ZipMagic magic;
	stream::readZipMagic(stream, magic);

	if (magic == ZIP_MAGIC_DISK_TRAILER)
	{
		// We got lucky and found the trailer right at the end of the file
		return startPosition;
	}

	// Trailer not found right at the end of the file.
	// Search for it, starting at the end of the file in backwards direction

	// ZIP comments havea 2-byte size descriptor, so the maximum size of the comment is 65k
    const SeekableStream::position_type maxCommentSize = 0x10000;

	// Allocate a buffer to hold the data to be searched
    const SeekableStream::position_type bufshift = 6;
    const SeekableStream::position_type bufsize = maxCommentSize >> bufshift;
    unsigned char buffer[bufsize];

	// Mark the end searching point in the file
    SeekableStream::position_type searchEndPos = (maxCommentSize < startPosition) ? startPosition - maxCommentSize : 0;
    SeekableStream::position_type position = startPosition;

    while (position != searchEndPos)
    {
		StreamBase::size_type bytesToRead = std::min(bufsize, position - searchEndPos);
		position -= bytesToRead;

		// Go the current search position, load the data and search it
		stream.seek(position);
		StreamBase::size_type size = stream.read(buffer, bytesToRead);

		// Search for the magic value and return its position on success
		unsigned char* p = buffer + size;

		while (p != buffer)
		{
			--p;

			magic.value[3] = magic.value[2];
			magic.value[2] = magic.value[1];
			magic.value[1] = magic.value[0];
			magic.value[0] = *p;

			if (magic == ZIP_MAGIC_DISK_TRAILER)
			{
				return position + (p - buffer);
			}
		}
    }

	// Zip magic not found
    return 0;
}

} // namespace
