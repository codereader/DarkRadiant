/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

#include "bytestreamutils.h"
#include "idatastream.h"
#include <algorithm>

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

}

// Various convenience functions, reading Zip structures from an InputStream
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

} // namespace

namespace archive
{

/* A. Local file header */
struct zip_file_header
{
	ZipMagic z_magic; /* local file header signature (0x04034b50) */
	ZipVersion z_extract; /* version needed to extract */
  unsigned short z_flags; /* general purpose bit flag */
  unsigned short z_compr; /* compression method */
	ZipDosTime z_dostime; /* last mod file time (dos format) */
  unsigned int z_crc32; /* crc-32 */
  unsigned int z_csize; /* compressed size */
  unsigned int z_usize; /* uncompressed size */
  unsigned short z_namlen; /* filename length (null if stdin) */
  unsigned short z_extras; /* extra field length */
  /* followed by filename (of variable size) */
  /* followed by extra field (of variable size) */
};

inline void istream_read_zip_file_header(SeekableInputStream& istream, zip_file_header& file_header)
{
	stream::readZipMagic(istream, file_header.z_magic);
	stream::readZipVersion(istream, file_header.z_extract);
  file_header.z_flags = istream_read_uint16_le(istream);
  file_header.z_compr = istream_read_uint16_le(istream);
	stream::readZipDosTime(istream, file_header.z_dostime);
  file_header.z_crc32 = istream_read_uint32_le(istream);
  file_header.z_csize = istream_read_uint32_le(istream);
  file_header.z_usize = istream_read_uint32_le(istream);
  file_header.z_namlen = istream_read_uint16_le(istream);
  file_header.z_extras = istream_read_uint16_le(istream);
  istream.seek(file_header.z_namlen + file_header.z_extras, SeekableInputStream::cur);
};

/* B. data descriptor
 * the data descriptor exists only if bit 3 of z_flags is set. It is byte aligned
 * and immediately follows the last byte of compressed data. It is only used if
 * the output media of the compressor was not seekable, eg. standard output.
 */
const ZipMagic ZIP_MAGIC_FILE_TRAILER('P', 'K', 0x07, 0x08);

struct zip_file_trailer
{
	ZipMagic z_magic;
  unsigned int z_crc32; /* crc-32 */
  unsigned int z_csize; /* compressed size */
  unsigned int z_usize; /* uncompressed size */
};

inline void istream_read_zip_file_trailer(InputStream& istream, zip_file_trailer& file_trailer)
{
	stream::readZipMagic(istream, file_trailer.z_magic);
  file_trailer.z_crc32 = istream_read_uint32_le(istream);
  file_trailer.z_csize = istream_read_uint32_le(istream);
  file_trailer.z_usize = istream_read_uint32_le(istream);
};


/* C. central directory structure:
    [file header] . . . end of central dir record
*/

/* directory file header
 * - a single entry including filename, extras and comment may not exceed 64k.
 */

const ZipMagic ZIP_MAGIC_ROOT_DIR_ENTRY('P', 'K', 0x01, 0x02);

struct zip_root_dirent
{
	ZipMagic z_magic;
	ZipVersion z_encoder;  /* version made by */
	ZipVersion z_extract;  /* version need to extract */
  unsigned short z_flags;  /* general purpose bit flag */
  unsigned short z_compr;  /* compression method */
	ZipDosTime z_dostime;  /* last mod file time&date (dos format) */
  unsigned int z_crc32;  /* crc-32 */
  unsigned int z_csize;  /* compressed size */
  unsigned int z_usize;  /* uncompressed size */
  unsigned short z_namlen; /* filename length (null if stdin) */
  unsigned short z_extras;  /* extra field length */
  unsigned short z_comment; /* file comment length */
  unsigned short z_diskstart; /* disk number of start (if spanning zip over multiple disks) */
  unsigned short z_filetype;  /* internal file attributes, bit0 = ascii */
  unsigned int z_filemode;  /* extrnal file attributes, eg. msdos attrib byte */
  unsigned int z_off;    /* relative offset of local file header, seekval if singledisk */
  /* followed by filename (of variable size) */
  /* followed by extra field (of variable size) */
  /* followed by file comment (of variable size) */
};

inline void istream_read_zip_root_dirent(SeekableInputStream& istream, zip_root_dirent& root_dirent)
{
	stream::readZipMagic(istream, root_dirent.z_magic);
	stream::readZipVersion(istream, root_dirent.z_encoder);
	stream::readZipVersion(istream, root_dirent.z_extract);
  root_dirent.z_flags = istream_read_uint16_le(istream);
  root_dirent.z_compr = istream_read_uint16_le(istream);
	stream::readZipDosTime(istream, root_dirent.z_dostime);
  root_dirent.z_crc32 = istream_read_uint32_le(istream);
  root_dirent.z_csize = istream_read_uint32_le(istream);
  root_dirent.z_usize = istream_read_uint32_le(istream);
  root_dirent.z_namlen = istream_read_uint16_le(istream);
  root_dirent.z_extras = istream_read_uint16_le(istream);
  root_dirent.z_comment = istream_read_uint16_le(istream);
  root_dirent.z_diskstart = istream_read_uint16_le(istream);
  root_dirent.z_filetype = istream_read_uint16_le(istream);
  root_dirent.z_filemode = istream_read_uint32_le(istream);
  root_dirent.z_off = istream_read_uint32_le(istream);
  istream.seek(root_dirent.z_namlen + root_dirent.z_extras + root_dirent.z_comment, SeekableInputStream::cur);
}

  /* end of central dir record */
const ZipMagic ZIP_MAGIC_DISK_TRAILER('P', 'K', 0x05, 0x06);
const unsigned int disk_trailer_length = 22;
struct zip_disk_trailer
{
	ZipMagic z_magic;
  unsigned short z_disk;  /* number of this disk */
  unsigned short z_finaldisk; /* number of the disk with the start of the central dir */
  unsigned short z_entries; /* total number of entries in the central dir on this disk */
  unsigned short z_finalentries; /* total number of entries in the central dir */
  unsigned int z_rootsize; /* size of the central directory */
  unsigned int z_rootseek; /* offset of start of central directory with respect to *
                        * the starting disk number */
  unsigned short z_comment;  /* zipfile comment length */
  /* followed by zipfile comment (of variable size) */
};

inline void istream_read_zip_disk_trailer(SeekableInputStream& istream, zip_disk_trailer& disk_trailer)
{
	stream::readZipMagic(istream, disk_trailer.z_magic);
  disk_trailer.z_disk = istream_read_uint16_le(istream);
  disk_trailer.z_finaldisk = istream_read_uint16_le(istream);
  disk_trailer.z_entries = istream_read_uint16_le(istream);
  disk_trailer.z_finalentries = istream_read_uint16_le(istream);
  disk_trailer.z_rootsize = istream_read_uint32_le(istream);
  disk_trailer.z_rootseek = istream_read_uint32_le(istream);
  disk_trailer.z_comment = istream_read_uint16_le(istream);
  istream.seek(disk_trailer.z_comment, SeekableInputStream::cur);
}

inline SeekableStream::position_type pkzip_find_disk_trailer(SeekableInputStream& istream)
{
  istream.seek(0, SeekableInputStream::end);
  SeekableStream::position_type start_position = istream.tell();
  if(start_position < disk_trailer_length)
    return 0;
  start_position -= disk_trailer_length;

  istream.seek(start_position);
	ZipMagic magic;
	stream::readZipMagic(istream, magic);

	if (magic == ZIP_MAGIC_DISK_TRAILER)
	{
		return start_position;
	}

    const SeekableStream::position_type max_comment = 0x10000;
    const SeekableStream::position_type bufshift = 6;
    const SeekableStream::position_type bufsize = max_comment >> bufshift;
    unsigned char buffer[bufsize];

    SeekableStream::position_type search_end = (max_comment < start_position) ? start_position - max_comment : 0;
    SeekableStream::position_type position = start_position;
    while(position != search_end)
    {
      StreamBase::size_type to_read = std::min(bufsize, position - search_end);
      position -= to_read;

      istream.seek(position);
      StreamBase::size_type size = istream.read(buffer, to_read);

      unsigned char* p = buffer + size;
      while(p != buffer)
      {
        --p;
        magic.value[3] = magic.value[2];
        magic.value[2] = magic.value[1];
        magic.value[1] = magic.value[0];
        magic.value[0] = *p;
        if(magic == ZIP_MAGIC_DISK_TRAILER)
        {
          return position + (p - buffer);
        }
      }
    }
    return 0;
}

} // namespace
