#pragma once

#include <string>
#include <boost/shared_ptr.hpp>
#include <GL/glew.h>

/**
 * \brief
 * Basic interface for all GL textures.
 *
 * This interface represents a texture which is bound to OpenGL, with an OpenGL
 * texture number (as returned from glGenTextures()). This may be a 2D, 3D, cube
 * map or any other kind of texture object supported by OpenGL.
 */
class Texture
{
public:

    /**
     * \brief
     * Constant indicating an invalid texture size.
     */
    const static unsigned INVALID_SIZE = 0;

    /**
     * \brief
     * Return the string name of this texture.
     */
    virtual std::string getName() const = 0;

    /**
     * \brief
     * Return the GL texture identifier for this texture.
     */
    virtual GLuint getGLTexNum() const = 0;

    /**
     * \brief
     * Return the width of this texture in pixels. May return INVALID_SIZE if
     * this texture does not have a valid size.
     */
    virtual unsigned getWidth() const = 0;

    /**
     * \brief
     * Return the height of this texture in pixels. May return INVALID_SIZE if
     * this texture does not have a valid size.
     */
    virtual unsigned getHeight() const = 0;
};
typedef boost::shared_ptr<Texture> TexturePtr;

