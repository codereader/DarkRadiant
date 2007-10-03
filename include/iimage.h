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

#if !defined(INCLUDED_IIMAGE_H)
#define INCLUDED_IIMAGE_H

#include "imodule.h"

typedef unsigned char byte;

class Image
{
public:
  virtual byte* getRGBAPixels() const = 0;
  virtual unsigned int getWidth() const = 0;
  virtual unsigned int getHeight() const = 0;

  virtual int getSurfaceFlags() const
  {
    return 0;
  }
  virtual int getContentFlags() const
  {
    return 0;
  }
  virtual int getValue() const
  {
    return 0;
  }
};
typedef boost::shared_ptr<Image> ImagePtr;

class ArchiveFile;

class ImageLoader :
	public RegisterableModule
{
public:
	/* greebo: Loads an image from the given ArchiveFile class
	 * 
	 * @returns: NULL, if the load failed, the pointer to the image otherwise
	 */
	virtual ImagePtr load(ArchiveFile& file) const = 0;
	
	/* greebo: Gets the file extension of the supported image file type
	 * 
	 * @returns the lowercase extension (e.g. "tga"). 
	 */
	virtual std::string getExtension() const = 0;
	
	/* greebo: Retrieves the directory prefix needed to construct the
	 * full path to the image (mainly needed for the "dds/" prefix for DDS textures).
	 * 
	 * @returns: the lowercase prefix (e.g. "dds/").
	 */
	virtual std::string getPrefix() const {
		return "";
	}
}; // class ImageLoader

const std::string MODULE_IMAGELOADER("ImageLoader");

// Acquires the PatchCreator of the given type ("TGA", "JPG", "DDS", "BMP", etc.)
inline ImageLoader& GlobalImageLoader(const std::string& fileType) {
	boost::shared_ptr<ImageLoader> _imageLoader(
		boost::static_pointer_cast<ImageLoader>(
			module::GlobalModuleRegistry().getModule(MODULE_IMAGELOADER + fileType) // e.g. "ImageLoaderTGA"
		)
	);
	return *_imageLoader;
}

#endif // _IIMAGE_H
