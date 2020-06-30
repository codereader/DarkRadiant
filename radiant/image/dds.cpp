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
#include <algorithm>

#include "ifilesystem.h"
#include "iarchive.h"
#include "idatastream.h"

#include "ddslib.h"
#include "util/Noncopyable.h"
#include "RGBAImage.h"

namespace image
{

// Metadata for a single MipMap level
struct MipMapInfo
{
    std::size_t width;  // pixel width
    std::size_t height; // pixel height

    std::size_t size;   // memory size used by this mipmap

    std::size_t offset; // offset in _pixelData to the beginning of this mipmap

    MipMapInfo() :
        width(0),
        height(0),
        size(0),
        offset(0)
    {}
};
typedef std::vector<MipMapInfo> MipMapInfoList;

// Image subclass for DDS images
class DDSImage: public Image, public util::Noncopyable
{
    // The actual pixels
    mutable std::vector<uint8_t> _pixelData;

    // The compression format ID
    GLenum _format = 0;

    // Metadata for each mipmap. All pixel data is stored in _pixelData, with
    // the offset to each mipmap stored in the _mipMapInfo list.
    MipMapInfoList _mipMapInfo;

public:

    // Pass the required memory size to the constructor
    DDSImage(std::size_t size): _pixelData(size)
    {}

    void setFormat(GLenum format)
    {
        _format = format;
    }

    // Add a new mipmap with the given parameters and return a pointer to its
    // allocated byte data
    uint8_t* addMipMap(std::size_t width, std::size_t height,
                       std::size_t size, std::size_t offset)
    {
        // Create the MipMapInfo metadata and store it in our list
        MipMapInfo info;
        info.size = size;
        info.width = width;
        info.height = height;
        info.offset = offset;
        _mipMapInfo.push_back(info);

        // Return the absolute pointer to the new mipmap's byte data
        assert(offset < _pixelData.size());
        return _pixelData.data() + offset;
    }

    /* Image implementation */
    uint8_t* getPixels() const override { return _pixelData.data(); }
    std::size_t getWidth() const override { return _mipMapInfo[0].width; }
    std::size_t getHeight() const override { return _mipMapInfo[0].height; }

    /* BindableTexture implementation */
    TexturePtr bindTexture(const std::string& name) const
    {
        GLuint textureNum;

        debug::assertNoGlErrors();

        // Allocate a new texture number and store it into the Texture structure
        glGenTextures(1, &textureNum);
        glBindTexture(GL_TEXTURE_2D, textureNum);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);

        for (std::size_t i = 0; i < _mipMapInfo.size(); ++i)
        {
            const MipMapInfo& mipMap = _mipMapInfo[i];

            glCompressedTexImage2D(
                GL_TEXTURE_2D,
                static_cast<GLint>(i),
                _format,
                static_cast<GLsizei>(mipMap.width),
                static_cast<GLsizei>(mipMap.height),
                0,
                static_cast<GLsizei>(mipMap.size),
                _pixelData.data() + mipMap.offset
            );

            // Handle unsupported format error
            if (glGetError() == GL_INVALID_ENUM)
            {
                rError() << "[DDSImage] Unable to bind texture '" << name
                         << "': unsupported texture format " << _format
                         << std::endl;

                return TexturePtr();
            }

            debug::assertNoGlErrors();
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(_mipMapInfo.size() - 1));

        // Un-bind the texture
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create and return texture object
        BasicTexture2DPtr texObj(new BasicTexture2D(textureNum, name));
        texObj->setWidth(getWidth());
        texObj->setHeight(getHeight());

        debug::assertNoGlErrors();

        return texObj;
    }

    bool isPrecompressed() const {
        return true;
    }
};
typedef std::shared_ptr<DDSImage> DDSImagePtr;

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

    MipMapInfoList mipMapInfo;
    mipMapInfo.resize(mipMapCount);

    // Calculate the total memory requirements (greebo: DXT1 has 8 bytes per block)
    std::size_t blockBytes = (pixelFormat == DDS_PF_DXT1) ? 8 : 16;

    std::size_t size = 0;
    std::size_t offset = 0;

    for (std::size_t i = 0; i < mipMapCount; ++i) {
        // Create a new mipmap structure
        MipMapInfo& mipMap = mipMapInfo[i];

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
            rError() << "DDS file has unknown format (" << pixelFormat << ")"
                     << std::endl;
            break;
    };

    // Load the mipmaps into the allocated memory
    for (std::size_t i = 0; i < mipMapInfo.size(); ++i)
    {
        const MipMapInfo& mipMap = mipMapInfo[i];

        // Declare a new mipmap and store the offset
        uint8_t* mipMapBytes = image->addMipMap(mipMap.width, mipMap.height,
                                                mipMap.size, mipMap.offset);

        // Read the data into the DDSImage's memory
        std::size_t bytesRead = stream.read(
            reinterpret_cast<byteType*>(mipMapBytes), mipMap.size
        );
        assert(bytesRead == mipMap.size);
    }

    return image;
}

ImagePtr LoadDDS(ArchiveFile& file) {
    return LoadDDSFromStream(file.getInputStream());
}

ImagePtr DDSLoader::load(ArchiveFile& file) const
{
    // Pass the call to the according load function
    return LoadDDS(file);
}

ImageTypeLoader::Extensions DDSLoader::getExtensions() const
{
    Extensions extensions;
    extensions.push_back("dds");
    return extensions;
}

}
