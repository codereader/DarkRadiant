#ifndef DDS_IMAGE_H_
#define DDS_IMAGE_H_

#include <vector>
#include "igl.h"

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

	// The compression format ID
	GLuint _format;

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

	void setFormat(GLuint format) {
		_format = format;
	}

	/**
	 * greebo: Declares a new mip map to be added to the internal
	 * structure.
	 */
	void addMipMap(std::size_t width, std::size_t height, std::size_t size, std::size_t offset) {
		MipMapInfo info;

		info.size = size;
		info.width = width;
		info.height = height;
		info.offset = offset;

		_mipMapInfo.push_back(info);
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

	virtual GLuint downloadTextureToGL() {
		GLuint textureNum;

		// Allocate a new texture number and store it into the Texture structure
		glGenTextures(1, &textureNum);
		glBindTexture(GL_TEXTURE_2D, textureNum);

		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);

		for (unsigned int i = 0; i < _mipMapInfo.size(); ++i) {
			const MipMapInfo& mipMap = _mipMapInfo[i];

			glCompressedTexImage2D(GL_TEXTURE_2D, i, _format, 
				mipMap.width, mipMap.height, 0, 
				mipMap.size, _pixelData + mipMap.offset );
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, _mipMapInfo.size() - 1);

		// Un-bind the texture
		glBindTexture(GL_TEXTURE_2D, 0);

		return textureNum;
	}

	bool isPrecompressed() const {
		return true;
	}
};
typedef boost::shared_ptr<DDSImage> DDSImagePtr;

#endif /* DDS_IMAGE_H_ */
