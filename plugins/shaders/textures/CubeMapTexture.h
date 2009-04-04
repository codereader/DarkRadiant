#pragma once

#include <Texture.h>

namespace shaders
{

/**
 * \brief
 * Implementation of Texture for a cube map texture.
 */
class CubeMapTexture
: public Texture
{
    // GL texture number
    GLuint _texNum;

    // Display name
    std::string _name;

public:

    /**
     * \brief
     * Construct a CubeMapTexture with the given texture number and name.
     */
    CubeMapTexture(GLuint texNum = 0, const std::string& name = "")
    : _texNum(texNum), _name(name)
    { }

    /* Texture implementation */
    
    std::string getName() const
    {
        return _name;
    }

    GLuint getGLTexNum() const
    {
        return _texNum;
    }

    unsigned getWidth() const
    {
        return INVALID_SIZE;
    }

    unsigned getHeight() const
    {
        return INVALID_SIZE;
    }
};

}
