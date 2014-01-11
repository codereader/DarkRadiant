#include "DDSImage.h"

#include <iostream>
#include "BasicTexture2D.h"

TexturePtr DDSImage::bindTexture(const std::string& name) const
{
    GLuint textureNum;

    GlobalOpenGL().assertNoErrors();

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
            _pixelData + mipMap.offset
        );

        // Handle unsupported format error
        if (glGetError() == GL_INVALID_ENUM)
        {
            std::cerr << "[DDSImage] Unable to bind texture '"
                      << name << "'; unsupported texture format"
                      << std::endl;

            return TexturePtr();
        }

        GlobalOpenGL().assertNoErrors();
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(_mipMapInfo.size() - 1));

    // Un-bind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create and return texture object
    BasicTexture2DPtr texObj(new BasicTexture2D(textureNum, name));
    texObj->setWidth(getWidth(0));
    texObj->setHeight(getHeight(0));

    GlobalOpenGL().assertNoErrors();

    return texObj;
}

void DDSImage::addMipMap(std::size_t width,
                         std::size_t height,
                         std::size_t size,
                         std::size_t offset)
{
    MipMapInfo info;

    info.size = size;
    info.width = width;
    info.height = height;
    info.offset = offset;

    _mipMapInfo.push_back(info);
}

