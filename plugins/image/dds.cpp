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

#include "dds.h"

#include <stdlib.h>

#include "ifilesystem.h"
#include "iarchive.h"
#include "idatastream.h"

#include "ddslib.h"
#include "imagelib.h"
#include "DDSImage.h"

DDSImagePtr LoadDDSFromStream(InputStream& stream)
{
	int width(0), height(0);
	ddsPF_t pixelFormat;

	// Load the header
	typedef StreamBase::byte_type byteType;
	DDSHeader header;
	stream.read(reinterpret_cast<byteType*>(&header), sizeof(header));

	if (DDSGetInfo(&header, &width, &height, &pixelFormat) == -1) {
		return DDSImagePtr();
	}

	// Get the number of mipmaps from the file
	std::size_t mipMapCount = (header.flags & DDSD_MIPMAPCOUNT) ? header.mipMapCount : 1;

	DDSImage::MipMapInfoList mipMapInfo;
	mipMapInfo.resize(mipMapCount);
	
	// Calculate the total memory requirements (greebo: DXT1 has 8 bytes per block)
	std::size_t blockBytes = (pixelFormat == DDS_PF_DXT1) ? 8 : 16;

	std::size_t size = 0;
	std::size_t offset = 0;

	for (std::size_t i = 0; i < mipMapCount; ++i) {
		// Create a new mipmap structure
		DDSImage::MipMapInfo& mipMap = mipMapInfo[i];

		mipMap.offset = offset;
		mipMap.width = width;
		mipMap.height = height;
		mipMap.size = std::max( width, 4 ) / 4 * std::max( height, 4 ) / 4 * blockBytes;

		// Update the offset for the next mipmap
		offset += mipMap.size;

		// Increase the size counter
		size += mipMap.size;

		// Go to the next mipmap
		width = (width+1) >> 1;
		height = (height+1) >> 1;
	}
	
	// Allocate a new DDS image with that size
	DDSImagePtr image(new DDSImage(size));

	// Set the format of this DDS image
	switch (pixelFormat)
	{
		case DDS_PF_DXT1:
			image->setFormat(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT);
			break;
		case DDS_PF_DXT3:
			image->setFormat(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT);
			break;
		case DDS_PF_DXT5:
			image->setFormat(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);
			break;
		default:
			break;
	};

	// Load the mipmaps into the allocated memory
	for (std::size_t i = 0; i < mipMapInfo.size(); ++i) {
		const DDSImage::MipMapInfo& mipMap = mipMapInfo[i];

		// Declare a new mipmap and store the offset
		image->addMipMap(mipMap.width, mipMap.height, mipMap.size, mipMap.offset);

		// Read the data into the DDSImage's memory
		std::size_t bytesRead =	stream.read(reinterpret_cast<byteType*>(image->getMipMapPixels(i)), mipMap.size);
		assert(bytesRead == mipMap.size);
	}

	return image;
}

ImagePtr LoadDDS(ArchiveFile& file) {
	return LoadDDSFromStream(file.getInputStream());
}
