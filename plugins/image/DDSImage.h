#ifndef DDS_IMAGE_H_
#define DDS_IMAGE_H_

#include <vector>

#include "imagelib.h"
#include <boost/noncopyable.hpp>

class DDSImage : 
	public Image,
	public boost::noncopyable
{
	// The actual pixels
	byte* _pixelData;

	// The amount of memory used by the image data
	std::size_t _memSize;

	struct MipMapInfo 
	{
		std::size_t width;	// pixel width
		std::size_t height; // pixel height
		std::size_t bpp;	// bytes per pixel

		std::size_t offset;	// offset in _pixelData to the beginning of this mipmap

		MipMapInfo() :
			width(0),
			height(0),
			bpp(0),
			offset(0)
		{}
	};
	typedef std::vector<MipMapInfo> MipMapInfoList;

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

	/**
	 * greebo: Declares a new mip map to be added to the internal
	 * memory calculation. The actual memory is not allocated by
	 * calling this, use the allocateMipMapMemory() for that purpose.
	 */
	void declareMipMap(std::size_t width, std::size_t height, std::size_t bpp) {
		MipMapInfo info;

		info.width = width;
		info.height = height;
		info.bpp = bpp;

		_mipMapInfo.push_back(info);
	}

	/**
	 * greebo: Removes all mipmap declarations. This leaves this class
	 * with 0 mipmaps and no allocated memory.
	 */
	void clearMipMapDeclarations() {
		// Free previously allocated memory
		releaseMemory();

		// Clear mipmap declarations
		_mipMapInfo.clear();
	}

	/**
	 * Allocates the memory as declared in the constructor.
	 * This will release any previously allocated memory.
	 */
	void allocateMemory() {
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

	virtual std::size_t getNumMipMaps() const {
		return _mipMapInfo.size();
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

	bool isPrecompressed() const {
		return true;
	}

private:
	/**
	 * Returns the total number of bytes required to store
	 * all declared mipmaps.
	 */
	std::size_t getTotalMemory() {
		std::size_t memory(0);

		for (MipMapInfoList::const_iterator i = _mipMapInfo.begin(); 
			 i != _mipMapInfo.end(); ++i)
		{
			memory += i->width * i->height * i->bpp;
		}

		return memory;
	}
};
typedef boost::shared_ptr<DDSImage> DDSImagePtr;

#endif /* DDS_IMAGE_H_ */
