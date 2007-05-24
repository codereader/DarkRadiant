#ifndef OGGFILESTREAM_H_
#define OGGFILESTREAM_H_

#include <iostream>

/** greebo: A wrapper class for use with the ov_open_callbacks() method
 * 			in vorbsfile.h. This provides the four callback
 * 			functions required by the OGG libs.
 * 
 * 			Parts of this code has been written after 
 * 			http://www.devmaster.net/articles/openal-ogg-file/
 */
namespace sound {

class OggFileStream
{
	ScopedArchiveBuffer& _source;
	
	unsigned char* _curPtr;
public:
	OggFileStream(ScopedArchiveBuffer& source) :
		_source(source)
	{
		// Set the pointer to the beginning of the buffer
		_curPtr = _source.buffer;
	}
	
	static std::size_t oggReadFunc(void* ptr, std::size_t byteSize, 
								   std::size_t sizeToRead, void* datasource)
	{
		OggFileStream* self = reinterpret_cast<OggFileStream*>(datasource);
		
		// Calculate how much we need to read.  
		// This can be sizeToRead*byteSize or less depending on how near the EOF marker we are
		std::size_t spaceToEOF = self->_source.buffer + self->_source.length - self->_curPtr;
		std::size_t	actualSizeToRead = 
			((sizeToRead*byteSize) < spaceToEOF) ? (sizeToRead*byteSize) : spaceToEOF;
		
		if (actualSizeToRead > 0) {
			// Copy the data
			memcpy(ptr, self->_curPtr, actualSizeToRead);
			// Increase the local pointer
			self->_curPtr += actualSizeToRead;
		}
		
		// Return how much we read (in the same way fread would)
		return actualSizeToRead;
	}
	
	static int oggSeekFunc(void* datasource, ogg_int64_t offset, int whence) {
		return -1; // not seekable
	}
	
	static int oggCloseFunc(void* datasource) {
		return 0;
	}
	
	static long oggTellFunc(void* datasource) {
		return 0;
	}
};

} // namespace sound

#endif /*OGGFILESTREAM_H_*/
