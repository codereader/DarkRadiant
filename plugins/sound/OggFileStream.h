#ifndef OGGFILESTREAM_H_
#define OGGFILESTREAM_H_

/** greebo: A wrapper class for use with the ov_open_callbacks() method
 * 			in vorbsfile.h. This provides the four callback
 * 			functions required by the OGG libs, simulating a file stream.
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
		std::size_t bytesRequested = sizeToRead*byteSize; 
		std::size_t bytesLeft = self->_source.buffer + self->_source.length - self->_curPtr;
		
		// Clamp the number of bytes to read to the number of bytes left
		std::size_t	actualSizeToRead = (bytesRequested < bytesLeft) ? bytesRequested : bytesLeft;
		
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
		OggFileStream* self = reinterpret_cast<OggFileStream*>(datasource);
		
		// Goto where we wish to seek to
		switch (whence)	{
			case SEEK_SET: 
				// Seek to the start of the data file
				self->_curPtr = self->_source.buffer + offset;
				break;
			case SEEK_CUR: 
				// Seek relatively to the current position
				self->_curPtr += offset;
				break;
			case SEEK_END: 
				// Seek the end of the file
				self->_curPtr = self->_source.buffer + self->_source.length;
				break;
			default:
				// Unknown SEEK case
				break;
		};
		
		// Clamp the curPtr to allowed values
		if (self->_curPtr > self->_source.buffer + self->_source.length) {
			self->_curPtr = self->_source.buffer + self->_source.length;
		}
		
		return 0;
	}
	
	static int oggCloseFunc(void* datasource) {
		return 1;
	}
	
	static long oggTellFunc(void* datasource) {
		OggFileStream* self = reinterpret_cast<OggFileStream*>(datasource);
		return static_cast<long>(self->_curPtr - self->_source.buffer);
	}
};

} // namespace sound

#endif /*OGGFILESTREAM_H_*/
