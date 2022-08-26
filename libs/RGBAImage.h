#pragma once

#include "igl.h"
#include "iimage.h"
#include "BasicTexture2D.h"
#include <memory>
#include "util/Noncopyable.h"
#include "debugging/gl.h"

namespace image
{

/// A single pixel with 8-bit RGBA components
struct RGBAPixel
{
	uint8_t red, green, blue, alpha;
};

/**
 * An RGBA image represents a single-mipmap image with certain
 * dimensions. The memory for the actual pixelmap is allocated
 * and de-allocated automatically.
 */
class RGBAImage :
	public Image,
	public util::Noncopyable
{
	std::size_t _width;
	std::size_t _height;

public:
	RGBAPixel* pixels;

    /// Construct image and initialise internal storage
	RGBAImage(std::size_t width, std::size_t height):
		_width(width),
		_height(height),
		pixels(new RGBAPixel[_width * _height])
	{}

	~RGBAImage()
	{
		delete[] pixels;
	}

    /* Image implementation */
	uint8_t* getPixels() const override
	{
		return reinterpret_cast<byte*>(pixels);
	}
	std::size_t getWidth(std::size_t = 0) const override { return _width; }
	std::size_t getHeight(std::size_t = 0) const override { return _height; }
    std::size_t getLevels() const override { return 1; }
    GLenum getGLFormat() const override { return GL_RGBA; }

    /* BindableTexture implementation */
    TexturePtr bindTexture(const std::string& name, Role role) const override
    {
		GLuint textureNum;

        debug::assertNoGlErrors();

		// Allocate a new texture number and store it into the Texture structure
		glGenTextures(1, &textureNum);
		glBindTexture(GL_TEXTURE_2D, textureNum);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		// Upload the image to OpenGL, choosing an internal format based on role
        GLint format = GL_RGBA8;
        if (role == Role::NORMAL_MAP) {
            format = GL_RG8;
        }

        // Download image and set up mipmaps and filtering
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, static_cast<GLint>(getWidth()),
                          static_cast<GLint>(getHeight()), GL_RGBA, GL_UNSIGNED_BYTE, getPixels());

        // Un-bind the texture
		glBindTexture(GL_TEXTURE_2D, 0);

        // Construct texture object
        BasicTexture2DPtr tex2DObject(new BasicTexture2D(textureNum, name));
        tex2DObject->setWidth(getWidth());
        tex2DObject->setHeight(getHeight());

        debug::assertNoGlErrors();

		return tex2DObject;
	}

	bool isPrecompressed() const override
	{
		return false; // not compressed
	}
};
typedef std::shared_ptr<RGBAImage> RGBAImagePtr;

}
