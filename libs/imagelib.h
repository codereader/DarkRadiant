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

#if !defined(INCLUDED_IMAGELIB_H)
#define INCLUDED_IMAGELIB_H

#include "iimage.h"
#include "iarchive.h"
#include "idatastream.h"
#include <stdlib.h>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

struct RGBAPixel
{
  unsigned char red, green, blue, alpha;
};

/**
 * An RGBA image represents a single-mipmap image with certain
 * dimensions. The memory for the actual pixelmap is allocated
 * and de-allocated automatically.
 */
class RGBAImage : 
	public Image,
	public boost::noncopyable
{
public:
	RGBAPixel* pixels;

	std::size_t width;
	std::size_t height;
	
	RGBAImage(std::size_t _width, std::size_t _height) : 
		pixels(new RGBAPixel[_width * _height]), 
		width(_width), 
		height(_height)
	{}

	~RGBAImage() {
		delete[] pixels;
	}

	virtual std::size_t getNumMipMaps() const {
		return 1; // we only have one mipmap at all times
	}

	virtual byte* getMipMapPixels(std::size_t mipMapIndex) const {
		assert(mipMapIndex == 0); // only one mipmap is allowed here

		return reinterpret_cast<byte*>(pixels);
	}

	virtual std::size_t getWidth(std::size_t mipMapIndex) const {
		assert(mipMapIndex == 0); // only one mipmap is allowed here

		return width;
	}

	virtual std::size_t getHeight(std::size_t mipMapIndex) const {
		assert(mipMapIndex == 0); // only one mipmap is allowed here

		return height;
	}

	virtual GLuint downloadTextureToGL() {
		GLuint textureNum;

		// Allocate a new texture number and store it into the Texture structure
		glGenTextures(1, &textureNum);
		glBindTexture(GL_TEXTURE_2D, textureNum);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

		// Download the image to OpenGL
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, 
			getWidth(0), getHeight(0), GL_RGBA, GL_UNSIGNED_BYTE, 
			getMipMapPixels(0)
		);

		// Un-bind the texture
		glBindTexture(GL_TEXTURE_2D, 0);

		return textureNum;
	}

	bool isPrecompressed() const {
		return false; // not compressed
	}
};
typedef boost::shared_ptr<RGBAImage> RGBAImagePtr;


inline InputStream::byte_type* ArchiveFile_loadBuffer(ArchiveFile& file, std::size_t& length)
{
  InputStream::byte_type* buffer = (InputStream::byte_type*)malloc(file.size() + 1);
  length = file.getInputStream().read(buffer, file.size());
  buffer[file.size()] = 0;
  return buffer;
}

inline void ArchiveFile_freeBuffer(InputStream::byte_type* buffer)
{
  free(buffer);
}

class ScopedArchiveBuffer
{
public:
  std::size_t length;
  InputStream::byte_type* buffer;

  ScopedArchiveBuffer(ArchiveFile& file)
  {
    buffer = ArchiveFile_loadBuffer(file, length);
  }
  ~ScopedArchiveBuffer()
  {
    ArchiveFile_freeBuffer(buffer);
  }
};

class PointerInputStream : public InputStream
{
  const byte* m_read;
public:
  PointerInputStream(const byte* pointer)
    : m_read(pointer)
  {
  }
  std::size_t read(byte* buffer, std::size_t length)
  {
    const byte* end = m_read + length;
    while(m_read != end)
    {
      *buffer++ = *m_read++;
    }
    return length;
  }
  void seek(std::size_t offset)
  {
    m_read += offset;
  }
  const byte* get()
  {
    return m_read;
  }
};

#endif
