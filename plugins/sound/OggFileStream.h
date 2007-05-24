#ifndef OGGFILESTREAM_H_
#define OGGFILESTREAM_H_

#include <iostream>

/** greebo: A wrapper class for use with the ov_open_callbacks() method
 * 			in vorbsfile.h. This provides the four callback
 * 			functions required by the OGG libs.
 */
namespace sound {

class OggFileStream
{
	ScopedArchiveBuffer& _source;
	
	unsigned char* curPtr;
public:
	OggFileStream(ScopedArchiveBuffer& source) :
		_source(source)
	{
		// Set the pointer to the beginning of the buffer
		curPtr = _source.buffer;
	}
	
	static std::size_t oggReadFunc(void* ptr, std::size_t size, 
								   std::size_t nmemb, void* datasource)
	{
		//std::cout << "ReadFunc: Ptr: " << ptr << ", Size: " << size << ", Datasource: " << datasource << "\n";
		OggFileStream* self = reinterpret_cast<OggFileStream*>(datasource);
		unsigned char* out = reinterpret_cast<unsigned char*>(ptr);
		
		for (std::size_t counter = 0; counter < size; counter++) {
			// Write the byte to the output buffer
			*out++ = *self->curPtr++;
		}
		
		// Are we at the end of the file, if yes, return 0 (EOF), 
		// otherwise return the number of read bytes
		return (self->curPtr >= self->_source.buffer + self->_source.length) ? 0 : size;
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
