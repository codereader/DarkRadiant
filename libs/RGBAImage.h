#pragma once

#include "igl.h"
#include "iimage.h"
#include "BasicTexture2D.h"
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

	~RGBAImage()
	{
		delete[] pixels;
	}

	virtual byte* getMipMapPixels(std::size_t mipMapIndex) const
	{
		assert(mipMapIndex == 0); // only one mipmap is allowed here

		return reinterpret_cast<byte*>(pixels);
	}

	virtual std::size_t getWidth(std::size_t mipMapIndex) const
	{
		assert(mipMapIndex == 0); // only one mipmap is allowed here

		return width;
	}

	virtual std::size_t getHeight(std::size_t mipMapIndex) const
	{
		assert(mipMapIndex == 0); // only one mipmap is allowed here

		return height;
	}

    /* BindableTexture implementation */
    TexturePtr bindTexture(const std::string& name) const
    {
		GLuint textureNum;

        GlobalOpenGL().assertNoErrors();

		// Allocate a new texture number and store it into the Texture structure
		glGenTextures(1, &textureNum);
		glBindTexture(GL_TEXTURE_2D, textureNum);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

		// Download the image to OpenGL
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA,
			static_cast<GLint>(getWidth(0)), static_cast<GLint>(getHeight(0)),
			GL_RGBA, GL_UNSIGNED_BYTE,
			getMipMapPixels(0)
		);

		// Un-bind the texture
		glBindTexture(GL_TEXTURE_2D, 0);

        // Construct texture object
        BasicTexture2DPtr tex2DObject(new BasicTexture2D(textureNum, name));
        tex2DObject->setWidth(getWidth(0));
        tex2DObject->setHeight(getHeight(0));

        GlobalOpenGL().assertNoErrors();

		return tex2DObject;
	}

	bool isPrecompressed() const
	{
		return false; // not compressed
	}
};
typedef boost::shared_ptr<RGBAImage> RGBAImagePtr;
