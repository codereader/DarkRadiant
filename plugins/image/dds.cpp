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
#include "stream/textstream.h"

#include "ddslib.h"
#include "imagelib.h"
#include "DDSImage.h"

DDSImagePtr LoadDDSFromBuffer(const byte* buffer)
{
	int width, height;
	ddsPF_t pixelFormat;

	const ddsBuffer_t* header = reinterpret_cast<const ddsBuffer_t*>(buffer);

	if (DDSGetInfo(header, &width, &height, &pixelFormat) == -1) {
		return DDSImagePtr();
	}

	// Get the number of mipmaps from the file
	std::size_t mipMapCount = (header->flags & DDSD_MIPMAPCOUNT) ? header->mipMapCount : 1;
	
	// Calculate the total memory requirements (greebo: DXT1 has 8 bytes per block)
	std::size_t blockBytes = (pixelFormat == DDS_PF_DXT1) ? 8 : 16;

	std::size_t size = 0;

	std::size_t x = width;
	std::size_t y = height;

	for (std::size_t i = 0; i < mipMapCount; ++i) {
		// Calculate this mipmap size
		size += std::max( 4, width ) / 4 * std::max( 4, height ) / 4 * blockBytes;

		// Go to the next mipmap
		width = (width+1) >> 1;
		height = (height+1) >> 1;
	}
	
	// Allocate a new DDS image with that size
	DDSImagePtr image(new DDSImage(size));

	// Declare a new mip map
	image->declareMipMap(width, height, 4);

	if (DDSDecompress(header, image->getMipMapPixels(0)) == -1) {
		return DDSImagePtr();
	}

	return image;
}

ImagePtr LoadDDS(ArchiveFile& file) {
	ScopedArchiveBuffer buffer(file);
	return LoadDDSFromBuffer(buffer.buffer);
}
