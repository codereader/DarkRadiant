#pragma once

#include <vector>
#include "igl.h"

#include "RGBAImage.h"
#include <boost/noncopyable.hpp>

class DDSImage :
	public Image,
	public boost::noncopyable
{
public:

	struct MipMapInfo
	{
		std::size_t width;	// pixel width
		std::size_t height; // pixel height

		std::size_t size;	// memory size used by this mipmap

		std::size_t offset;	// offset in _pixelData to the beginning of this mipmap

		MipMapInfo() :
			width(0),
			height(0),
			size(0),
			offset(0)
		{}
	};
	typedef std::vector<MipMapInfo> MipMapInfoList;

private:
	// The actual pixels
	byte* _pixelData;

	// The amount of memory used by the image data
	std::size_t _memSize;

	// The compression format ID
	GLuint _format;

	MipMapInfoList _mipMapInfo;

public:
	RGBAPixel* pixels;
	unsigned int width, height;

	// Pass the required memory size to the constructor
	DDSImage(std::size_t size) :
		_pixelData(NULL),
		_memSize(size)
	{
		allocateMemory();
	}

	~DDSImage() {
		releaseMemory();
	}

	void setFormat(GLuint format)
    {
		_format = format;
	}

	/**
	 * greebo: Declares a new mip map to be added to the internal
	 * structure.
	 */
	void addMipMap(std::size_t width,
                   std::size_t height,
                   std::size_t size,
                   std::size_t offset);

	/**
	 * Allocates the memory as declared in the constructor.
	 * This will release any previously allocated memory.
	 */
	void allocateMemory()
    {
		// Release memory first
		releaseMemory();

		_pixelData = new byte[_memSize];
	}

	void releaseMemory() {
		// Check if we have something to do at all
		if (_pixelData == NULL) return;

		delete[] _pixelData;
		_pixelData = NULL;
	}

	/**
	 * greebo: Returns the specified mipmap pixel data.
	 */
	virtual byte* getMipMapPixels(std::size_t mipMapIndex) const {
		assert(mipMapIndex < _mipMapInfo.size());

		return _pixelData + _mipMapInfo[mipMapIndex].offset;
	}

	/**
	 * greebo: Returns the dimension of the specified mipmap.
	 */
	virtual std::size_t getWidth(std::size_t mipMapIndex) const {
		assert(mipMapIndex < _mipMapInfo.size());

		return _mipMapInfo[mipMapIndex].width;
	}

	virtual std::size_t getHeight(std::size_t mipMapIndex) const {
		assert(mipMapIndex < _mipMapInfo.size());

		return _mipMapInfo[mipMapIndex].height;
	}

    /* BindableTexture implementation */
	TexturePtr bindTexture(const std::string& name) const;

	bool isPrecompressed() const {
		return true;
	}
};
typedef boost::shared_ptr<DDSImage> DDSImagePtr;
