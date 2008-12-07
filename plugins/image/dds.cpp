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

	if (DDSGetInfo(reinterpret_cast<ddsBuffer_t*>(const_cast<byte*>(buffer)), &width, &height, &pixelFormat) == -1) {
		return DDSImagePtr();
	}

	DDSImagePtr image(new DDSImage());

	// Declare a new mip map
	image->declareMipMap(width, height, 4);
	image->allocateMipMapMemory();

	if (DDSDecompress(reinterpret_cast<ddsBuffer_t*>(const_cast<byte*>(buffer)), image->getMipMapPixels(0)) == -1) {
		return DDSImagePtr();
	}

	return image;
}

ImagePtr LoadDDS(ArchiveFile& file) {
	ScopedArchiveBuffer buffer(file);
	return LoadDDSFromBuffer(buffer.buffer);
}
