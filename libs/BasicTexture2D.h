#pragma once

#include "Texture.h"

/**
 * \brief
 * Implementation of Texture for a 2D image texture.
 */
class BasicTexture2D
: public Texture
{
	// The GL bind number for use in OpenGL calls
	GLuint texture_number;
	
	// Texture Dimensions
	std::size_t _width, _height;

   // Texture name
   std::string _name;
	
public:

	// Constructor
	BasicTexture2D(GLuint texNum = 0, const std::string& name = "") 
   : texture_number(texNum),
     _name(name)
	{}
	
	~BasicTexture2D() {
		if (texture_number != 0) {
			// Remove this texture from openGL if it's still loaded
			glDeleteTextures(1, &texture_number);
		}
	}

    /**
     * \brief
     * Set the texture number.
     */
    void setGLTexNum(GLuint texnum)
    {
        texture_number = texnum;
    }

    /**
     * \brief
     * Set the image width.
     */
    void setWidth(unsigned width)
    {
        _width = width;
    }

    /**
     * \brief
     * Set the image height.
     */
    void setHeight(unsigned height)
    {
        _height = height;
    }

    /* Texture interface */
    std::string getName() const
    {
        return _name;
    }
    GLuint getGLTexNum() const
    {
        return texture_number;
    }
    unsigned getWidth() const
    {
        return _width;
    }
    unsigned getHeight() const
    {
        return _height;
    }

}; // class Texture 

typedef boost::shared_ptr<BasicTexture2D> BasicTexture2DPtr;

