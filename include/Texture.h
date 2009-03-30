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
     * Return the width of this texture in pixels.
     */
    virtual unsigned getWidth() const = 0;

    /**
     * \brief
     * Return the height of this texture in pixels.
     */
    virtual unsigned getHeight() const = 0;
};
typedef boost::shared_ptr<Texture> TexturePtr;

